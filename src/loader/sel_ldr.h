/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*
 * Copyright (c) 2012, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 *
 * This loader can only process NaCl object files as produced using
 * the NaCl toolchain.  Other ELF files will be rejected.
 *
 * The primary function, NaClAppLoadFile, parses an ELF file,
 * allocates memory, loads the relocatable image from the ELF file
 * into memory, and performs relocation.  NaClAppRun runs the
 * resultant program.
 *
 * This loader is written in C so that it can be used by C-only as
 * well as C++ applications.  Other languages should also be able to
 * use their foreign-function interfaces to invoke C code.
 *
 * This loader must be part of the NaCl TCB, since it directly handles
 * externally supplied input (the ELF file).  Any security
 * vulnerabilities in handling the ELF image, e.g., buffer or integer
 * overflows, can put the application at risk.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_LDR_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_LDR_H_ 1

#include <stdint.h>

/* Keys for auxiliary vector (auxv). */
#define AT_NULL         0   /* Terminating item in auxv array */
#define AT_ENTRY        9   /* Entry point of the executable */
#define AT_SYSINFO      32  /* System call entry point */

#include "src/main/zlog.h"
#include "src/platform/gio.h"
#include "src/loader/sel_util.h"
#include "src/loader/sel_rt.h"
#include "src/main/tools.h"
#include "src/main/etag.h"

EXTERN_C_BEGIN

#define NACL_SERVICE_PORT_DESCRIPTOR    3
#define NACL_SERVICE_ADDRESS_DESCRIPTOR 4

#define NACL_DEFAULT_STACK_MAX  (16 << 20)  /* main thread stack */

#define NACL_SANDBOX_CHROOT_FD  "SBX_D"

/*
 * helper macro. _element should be nap->mem_map[index], _addr - block
 * address (bytes), _size - block size (bytes), _prot - block protection
 */
#define SET_MEM_MAP_IDX(_element, _name, _addr, _size, _prot) \
  do {\
    _element.name = _name;\
    _element.page_num = (_addr) >> NACL_PAGESHIFT;\
    _element.npages = (_size) >> NACL_PAGESHIFT;\
    _element.prot = _prot;\
  } while(0)

enum MemMapIndices {
  LeftBumperIdx, /* includes NULL protector */
  TextIdx, /* includes trampoline */
  RODataIdx,
  HeapIdx, /* includes r/w data */
  StackIdx,
  SysDataIdx,
  RightBumperIdx,
  MemMapSize
};

struct MemBlock {
  char                  *name;    /* block name */
  intptr_t              page_num; /* system virtual address >> NACL_PAGESHIFT */
  size_t                npages;   /* number of pages */
  int                   prot;     /* mprotect attribute */
};

struct NaClAppThread;
struct NaClDynamicRegion;
struct NaClManifestProxy;
struct NaClSecureService;
struct NaClSecureReverseService;
struct NaClThreadInterface;  /* see sel_ldr_thread_interface.h */

struct NaClApp {
  /*
   * public, user settable prior to app start.
   */
  uint8_t                   addr_bits;
  uintptr_t                 stack_size;
  /*
   * stack_size is the maximum size of the (main) stack.  The stack
   * memory is eager allocated (mapped in w/o MAP_NORESERVE) so
   * there must be enough swap space; page table entries are not
   * populated (no MAP_POPULATE), so actual accesses will likely
   * incur page faults.
   */

  /*
   * Determined at load time; OS-determined.
   * Read-only after load, so accesses do not require locking.
   */
  uintptr_t                 mem_start;

  uintptr_t                 dispatch_thunk;

  /* only used for ET_EXEC:  for CS restriction */
  uintptr_t                 static_text_end;  /* relative to mem_start */
  /* ro after app starts. memsz from phdr */

  /*
   * The dynamic code area follows the static code area.  These fields
   * are both set to static_text_end if the dynamic code area has zero
   * size.
   */
  uintptr_t                 dynamic_text_start;
  uintptr_t                 dynamic_text_end;

  /*
   * rodata_start and data_start may be 0 if these segments are not
   * present in the executable.
   */
  uintptr_t                 rodata_start;  /* initialized data, ro */
  uintptr_t                 data_start;    /* initialized data/bss, rw */
  /*
   * Various region sizes must be a multiple of NACL_MAP_PAGESIZE
   * before the NaCl app can run.  The sizes from the ELF file
   * (p_filesz field) might not be -- that would waste space for
   * padding -- and while we could use p_memsz to specify padding, but
   * we will record the virtual addresses of the start of the segments
   * and figure out the gap between the p_vaddr + p_filesz of one
   * segment and p_vaddr of the next to determine padding.
   */

  uintptr_t                 data_end;
  /* see break_addr below */

  /*
   * initial_entry_pt is the first address in untrusted code to jump
   * to.  When using the IRT (integrated runtime), this is provided by
   * the IRT library, and user_entry_pt is the entry point in the user
   * executable.  Otherwise, initial_entry_pt is in the user
   * executable and user_entry_pt is zero.
   */
  uintptr_t                 initial_entry_pt;
  uintptr_t                 user_entry_pt;

  /*
   * bundle_size is the bundle alignment boundary for validation (16
   * or 32), so int is okay.  This value must be a power of 2.
   */
  int                       bundle_size; /* d'b: can be replaced with define.
                                            only need for NaClSandboxCodeAddr */

  /*
   * An array of NaCl syscall handlers. The length of the array must be
   * at least NACL_MAX_SYSCALLS.
   */
  struct NaClSyscallTableEntry *syscall_table;

  /* user memory map */
  struct MemBlock           mem_map[MemMapSize];

  /*
   * may reject nexes that are incompatible w/ dynamic-text in the near future
   */
  struct NaClDesc           *text_shm;
  /*
   * This records which pages in text_shm have been allocated.  When a
   * page is allocated, it is filled with halt instructions and then
   * made executable by untrusted code.
   */
  uint8_t                   *dynamic_page_bitmap;

  /*
   * The array of dynamic_regions is maintained in sorted order
   * Accesses must be protected by dynamic_load_mutex
   */
  struct NaClDynamicRegion  *dynamic_regions;
  int                       num_dynamic_regions;
  int                       dynamic_regions_allocated;

  /*
   * These variables are used for caching mapped writable views of the
   * dynamic text segment.  See CachedMapWritableText in nacl_text.c.
   * Accesses must be protected by dynamic_load_mutex
   */
  uint32_t                  dynamic_mapcache_offset;
  uint32_t                  dynamic_mapcache_size;
  uintptr_t                 dynamic_mapcache_ret;

  int                       skip_validator;

  uintptr_t                 break_addr;   /* user addr */
  /* data_end <= break_addr is an invariant */

  /* d'b: added fields {{ */
  struct SystemManifest     *system_manifest;
  uintptr_t                 heap_end; /* end of user heap */
  int                       validation_state; /* needs for the report */
  int                       skip_qualification; /* startup time variable */
  int                       quit_after_load; /* startup time variable */
  int                       handle_signals; /* startup time variable */
  int64_t                   storage_limit; /* hard limit for all zerovm i/o */
  void                      *user_tag; /* tag context for memory and channels */

  uintptr_t                 user_manifest; /* pointer to user manifest, points to user space, r */

  /* for channels */
  int                       node_id; /* own node id from manifest */
  char                      *node_name; /* own node name from manifest. available for user */

  /* fields taken from the natp */
  void                      *signal_stack; /* signal handling, registered with sigaltstack() */
  uintptr_t                 *syscall_args;
  uint32_t                  sysret; /* syscall return code */
  uintptr_t                 sys_tls;
  /* }} */
};

/*
 * Initializes a NaCl application with the default parameters
 * and the specified syscall table.
 *
 * nap is a pointer to the NaCl object that is being filled in.
 *
 * table is the NaCl syscall table. The syscall table must contain at least
 * NACL_MAX_SYSCALLS valid entries.
 *
 * Caution! Syscall handlers must be extremely careful with respect to
 * argument validation, including time-of-check vs time-of-use defense, etc.
 */
int NaClAppWithSyscallTableCtor(struct NaClApp               *nap,
                                struct NaClSyscallTableEntry *table) NACL_WUR;

int   NaClAppCtor(struct NaClApp  *nap) NACL_WUR;

/*
 * Loads a NaCl ELF file into memory in preparation for running it.
 *
 * gp is a pointer to a generic I/O object and should be a GioMem with
 * a memory buffer containing the file read entirely into memory if
 * the file system might be subject to race conditions (e.g., another
 * thread / process might modify a downloaded NaCl ELF file while we
 * are loading it here).
 *
 * nap is a pointer to the NaCl object that is being filled in.  it
 * should be properly constructed via NaClAppCtor.
 *
 * return value: one of the LOAD_* values defined in
 * nacl_error_code.h.  TODO: add some error detail string and hang
 * that off the nap object, so that more details are available w/o
 * incrementing verbosity (and polluting stdout).
 *
 * note: it may be necessary to flush the icache if the memory
 * allocated for use had already made it into the icache from another
 * NaCl application instance, and the icache does not detect
 * self-modifying code / data writes and automatically invalidate the
 * cache lines.
 */
void NaClAppLoadFile(struct Gio *gp, struct NaClApp *nap);

void  NaClAppPrintDetails(struct NaClApp  *nap,
                          struct Gio      *gp, int verbosity);

int NaClValidateCode(struct NaClApp *nap,
                     uintptr_t      guest_addr,
                     uint8_t        *data,
                     size_t         size) NACL_WUR;

/*
 * Validates that the code found at data_old can safely be replaced with
 * the code found at data_new.
 */
int NaClValidateCodeReplacement(struct    NaClApp *nap,
                                uintptr_t guest_addr,
                                uint8_t   *data_old,
                                uint8_t   *data_new,
                                size_t    size);

/*
 * Copies code from data_new to data_old in a thread-safe way
 */
int NaClCopyCode(struct NaClApp *nap, uintptr_t guest_addr,
                 uint8_t *data_old, uint8_t *data_new,
                 size_t size);

int NaClAddrIsValidEntryPt(struct NaClApp *nap,
                           uintptr_t      addr);

/*
 * Used to launch the main thread.  NB: calling thread may in the
 * future become the main NaCl app thread, and this function will
 * return only after the NaCl app main thread exits.  In such an
 * alternative design, NaClWaitForMainThreadToExit will become a
 * no-op.
 */
int NaClCreateMainThread(struct NaClApp     *nap) NACL_WUR;

int NaClWaitForMainThreadToExit(struct NaClApp  *nap);

/*
 * Used by syscall code.
 */
int32_t NaClCreateAdditionalThread(struct NaClApp *nap,
                                   uintptr_t      prog_ctr,
                                   uintptr_t      stack_ptr,
                                   uintptr_t      sys_tls,
                                   uint32_t       user_tls2) NACL_WUR;

void NaClLoadTrampoline(struct NaClApp *nap);

void NaClLoadSpringboard(struct NaClApp  *nap);

static const uintptr_t kNaClBadAddress = (uintptr_t) -1;

#ifndef NACL_NO_INLINE
#include "src/loader/sel_ldr-inl.h"
#endif

/*
 * Looks up a descriptor in the open-file table.  An additional
 * reference is taken on the returned NaClDesc object (if non-NULL).
 * The caller is responsible for invoking NaClDescUnref() on it when
 * done.
 */
struct NaClDesc *NaClGetDesc(struct NaClApp *nap,
                             int            d);

/*
 * Versions that are called while already holding the desc_mu lock
 */
struct NaClDesc *NaClGetDescMu(struct NaClApp *nap,
                               int            d);

void NaClSetDescMu(struct NaClApp   *nap,
                   int              d,
                   struct NaClDesc  *ndp);

int32_t NaClSetAvailMu(struct NaClApp   *nap,
                       struct NaClDesc  *ndp);

void NaClAppInitialDescriptorHookup(struct NaClApp  *nap);

void NaClFillMemoryRegionWithHalt(void *start, size_t size);

void NaClFillTrampolineRegion(struct NaClApp *nap);

void NaClFillEndOfTextRegion(struct NaClApp *nap);

int NaClMakeDispatchThunk(struct NaClApp *nap);

void NaClPatchOneTrampoline(struct NaClApp *nap,
                            uintptr_t target_addr);
/*
 * target is an absolute address in the source region.  the patch code
 * will figure out the corresponding address in the destination region
 * and modify as appropriate.  this makes it easier to specify, since
 * the target is typically the address of some symbol from the source
 * template.
 */
struct NaClPatch {
  uintptr_t           target;
  uint64_t            value;
};

struct NaClPatchInfo {
  uintptr_t           dst;
  uintptr_t           src;
  size_t              nbytes;

  struct NaClPatch    *abs16;
  size_t              num_abs16;

  struct NaClPatch    *abs32;
  size_t              num_abs32;

  struct NaClPatch    *abs64;
  size_t              num_abs64;

  uintptr_t           *rel64;
  size_t              num_rel64;
};

struct NaClPatchInfo *NaClPatchInfoCtor(struct NaClPatchInfo *self);

void NaClApplyPatchToMemory(struct NaClPatchInfo *patch);

int NaClThreadContextCtor(struct NaClThreadContext  *ntcp,
                          struct NaClApp            *nap,
                          nacl_reg_t                prog_ctr,
                          nacl_reg_t                stack_ptr,
                          uint32_t                  tls_info);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_LDR_H_ */
