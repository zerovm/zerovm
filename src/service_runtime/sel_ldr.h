/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
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

#include "include/elf.h"
#include "src/platform/nacl_log.h"
#include "src/service_runtime/dyn_array.h"
#include "src/service_runtime/nacl_error_code.h"

#include "src/service_runtime/sel_mem.h"
#include "src/service_runtime/sel_util.h"
#include "src/service_runtime/sel_rt.h"
#include "src/utils/tools.h"

#include "src/service_runtime/nacl_cpuid.h" /* d'b: taken from validator */
//#include "src/validator/cpufeatures.h"

EXTERN_C_BEGIN

#define NACL_SERVICE_PORT_DESCRIPTOR    3
#define NACL_SERVICE_ADDRESS_DESCRIPTOR 4

#define NACL_DEFAULT_STACK_MAX  (16 << 20)  /* main thread stack */

#define NACL_SANDBOX_CHROOT_FD  "SBX_D"

struct NaClAppThread;
struct NaClDesc;  /* see src/desc/nacl_desc_base.h */
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
  int                       bundle_size;

  /*
   * An array of NaCl syscall handlers. The length of the array must be
   * at least NACL_MAX_SYSCALLS.
   */
  struct NaClSyscallTableEntry *syscall_table;

  NaClErrorCode             module_load_status;
  int                       module_may_start;

  /*
   * runtime info below, thread state, etc; initialized only when app
   * is run.  Mutex mu protects access to mem_map and other member
   * variables while the application is running and may be
   * multithreaded; thread, desc members have their own locks.  At
   * other times it is assumed that only one thread is
   * constructing/loading the NaClApp and that no mutual exclusion is
   * needed.
   */

  /*
   * memory map is in user addresses.
   */
  struct NaClVmmap          mem_map;

  /*
   * This is the effector interface object that is used to manipulate
   * NaCl apps by the objects in the NaClDesc class hierarchy.  This
   * is used by this NaClApp when making NaClDesc method calls from
   * syscall handlers.
   */
  struct NaClDescEffector   *effp; // #6 to remove

  /*
   * may reject nexes that are incompatible w/ dynamic-text in the near future
   */
  int                       use_shm_for_dynamic_text;
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

  /*
   * Monotonically increasing generation number used for deletion
   * Accesses must be protected by dynamic_load_mutex
   */
  int                       dynamic_delete_generation;

  int                       skip_validator;

  uintptr_t                 break_addr;   /* user addr */
  /* data_end <= break_addr is an invariant */

  /*
   * Thread table lock threads_mu is higher in the locking order than
   * the thread locks, i.e., threads_mu must be acqured w/o holding
   * any per-thread lock (natp->mu).
   */
  struct DynArray           threads;   /* NaClAppThread pointers */ // #12 to remove

  struct DynArray           desc_tbl;  /* NaClDesc pointers */

  /* d'b: added fields {{ */
  struct SystemManifest     *system_manifest;
  int32_t                   zvm_code; /* zerovm return code */
  /* nexe exit state. NULL == ok, otherwise terminated by signal/timeout */
  char                      zvm_state[SIGNAL_STRLEN + 1];
  int                       validation_state; /* needs for the report */
  int                       skip_qualification; /* startup time variable */
  int                       fuzzing_quit_after_load; /* startup time variable */
  int                       verbosity; /* startup time variable */
  int                       handle_signals; /* startup time variable */

  /* for channels */
  int                       node_id; /* own node id from manifest */
  char                      *node_name; /* own node name from manifest. available for user */

  /* fields required by "whole chunk" user memory management */
  int                       user_side_flag; /* 0 - trusted syscall, 1 - called from user side */
  int                       trusted_code; /* mark to catch source of signal */

  /* for validator */
  NaClCPUFeatures           cpu_features;

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
NaClErrorCode NaClAppLoadFile(struct Gio      *gp,
                              struct NaClApp  *nap) NACL_WUR;

NaClErrorCode NaClAppLoadFileDynamically(struct NaClApp *nap,
                                         struct Gio     *gio_file) NACL_WUR;

void  NaClAppPrintDetails(struct NaClApp  *nap,
                          struct Gio      *gp);

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

NaClErrorCode NaClValidateImage(struct NaClApp  *nap) NACL_WUR;

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
#include "src/service_runtime/sel_ldr-inl.h"
#endif

/*
 * Looks up a descriptor in the open-file table.  An additional
 * reference is taken on the returned NaClDesc object (if non-NULL).
 * The caller is responsible for invoking NaClDescUnref() on it when
 * done.
 */
struct NaClDesc *NaClGetDesc(struct NaClApp *nap,
                             int            d);

int32_t NaClSetAvail(struct NaClApp   *nap,
                     struct NaClDesc  *ndp);

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
