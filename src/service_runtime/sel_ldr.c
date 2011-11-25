/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */
#include "include/portability_io.h"
#include "include/portability_string.h"
#include "include/nacl_macros.h"

#include "src/gio/gio.h"
#include "src/platform/nacl_check.h"
#include "src/platform/nacl_exit.h"
#include "src/platform/nacl_log.h"
#include "src/platform/nacl_sync.h"
#include "src/platform/nacl_sync_checked.h"
#include "src/platform/nacl_time.h"

#include "src/desc/nacl_desc_base.h"
#include "src/desc/nacl_desc_conn_cap.h"
#include "src/desc/nacl_desc_imc.h"
#include "src/desc/nacl_desc_io.h"
#include "src/desc/nrd_xfer.h"

#include "src/fault_injection/fault_injection.h"

#include "src/gio/gio_nacl_desc.h"
#include "src/gio/gio_shm.h"
#include "src/service_runtime/arch/sel_ldr_arch.h"
#include "src/service_runtime/include/sys/fcntl.h"
#include "src/service_runtime/nacl_app.h"
#include "src/service_runtime/nacl_app_thread.h"
#include "src/service_runtime/nacl_desc_effector_ldr.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_syscall_common.h"
#include "src/service_runtime/nacl_syscall_handlers.h"
#include "src/service_runtime/sel_addrspace.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/sel_memory.h"

#include "src/service_runtime/sel_ldr_thread_interface.h"
#include "src/threading/nacl_thread_interface.h"

#include "src/service_runtime/include/sys/stat.h"
#include "src/service_runtime/include/sys/time.h"

#include "src/threading/nacl_thread_interface.h"

static int IsEnvironmentVariableSet(char const *env_name) {
  return NULL != getenv(env_name);
}

static int ShouldEnableDynamicLoading() {
  return !IsEnvironmentVariableSet("NACL_DISABLE_DYNAMIC_LOADING");
}

int NaClAppWithSyscallTableCtor(struct NaClApp               *nap,
                                struct NaClSyscallTableEntry *table) {
  struct NaClDescEffectorLdr  *effp;

  nap->addr_bits = NACL_MAX_ADDR_BITS;

  nap->stack_size = NACL_DEFAULT_STACK_MAX;

  nap->mem_start = 0;

#if (NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 \
     && NACL_BUILD_SUBARCH == 32 && __PIC__)
  nap->pcrel_thunk = 0;
#endif
#if (NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 \
     && NACL_BUILD_SUBARCH == 64)
  nap->dispatch_thunk = 0;
#endif

  nap->static_text_end = 0;
  nap->dynamic_text_start = 0;
  nap->dynamic_text_end = 0;
  nap->rodata_start = 0;
  nap->data_start = 0;
  nap->data_end = 0;

  nap->initial_entry_pt = 0;
  nap->user_entry_pt = 0;

  if (!DynArrayCtor(&nap->threads, 2)) {
    goto cleanup_none;
  }
  if (!DynArrayCtor(&nap->desc_tbl, 2)) {
    goto cleanup_threads;
  }
  if (!NaClVmmapCtor(&nap->mem_map)) {
    goto cleanup_desc_tbl;
  }

  effp = (struct NaClDescEffectorLdr *) malloc(sizeof *effp);
  if (NULL == effp) {
    goto cleanup_mem_map;
  }
  if (!NaClDescEffectorLdrCtor(effp, nap)) {
    goto cleanup_effp_free;
  }
  nap->effp = (struct NaClDescEffector *) effp;

  nap->use_shm_for_dynamic_text = ShouldEnableDynamicLoading();
  nap->text_shm = NULL;
  if (!NaClMutexCtor(&nap->dynamic_load_mutex)) {
    goto cleanup_effp_dtor;
  }
  nap->dynamic_page_bitmap = NULL;

  nap->dynamic_regions = NULL;
  nap->num_dynamic_regions = 0;
  nap->dynamic_regions_allocated = 0;
  nap->dynamic_delete_generation = 0;

  nap->dynamic_mapcache_offset = 0;
  nap->dynamic_mapcache_size = 0;
  nap->dynamic_mapcache_ret = 0;

  if (!NaClMutexCtor(&nap->mu)) {
    goto cleanup_dynamic_load_mutex;
  }
  if (!NaClCondVarCtor(&nap->cv)) {
    goto cleanup_mu;
  }

  nap->vm_hole_may_exist = 0;
  nap->threads_launching = 0;

  nap->syscall_table = table;

  nap->module_load_status = LOAD_STATUS_UNKNOWN;
  nap->module_may_start = 0;  /* only when secure_service != NULL */

  nap->ignore_validator_result = 0;
  nap->skip_validator = 0;
  nap->validator_stub_out_mode = 0;

  if (!NaClMutexCtor(&nap->threads_mu)) {
    goto cleanup_cv;
  }
  if (!NaClCondVarCtor(&nap->threads_cv)) {
    goto cleanup_threads_mu;
  }
  nap->num_threads = 0;
  if (!NaClMutexCtor(&nap->desc_mu)) {
    goto cleanup_threads_cv;
  }

  nap->running = 0;
  nap->exit_status = -1;

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  nap->code_seg_sel = 0;
  nap->data_seg_sel = 0;
#endif

  nap->enable_debug_stub = 0;
  nap->debug_stub_callbacks = NULL;

  return 1;

#if 0
 cleanup_desc_mu:
  NaClMutexDtor(&nap->desc_mu);
#endif
 cleanup_threads_cv:
  NaClCondVarDtor(&nap->threads_cv);
 cleanup_threads_mu:
  NaClMutexDtor(&nap->threads_mu);
 cleanup_cv:
  NaClCondVarDtor(&nap->cv);
 cleanup_mu:
  NaClMutexDtor(&nap->mu);
 cleanup_dynamic_load_mutex:
  NaClMutexDtor(&nap->dynamic_load_mutex);
 cleanup_effp_dtor:
  (*nap->effp->vtbl->Dtor)(nap->effp);
 cleanup_effp_free:
  free(nap->effp);
 cleanup_mem_map:
  NaClVmmapDtor(&nap->mem_map);
 cleanup_desc_tbl:
  DynArrayDtor(&nap->desc_tbl);
 cleanup_threads:
  DynArrayDtor(&nap->threads);
 cleanup_none:
  return 0;
}

int NaClAppCtor(struct NaClApp *nap) {
  return NaClAppWithSyscallTableCtor(nap, nacl_syscall);
}

size_t  NaClAlignPad(size_t val, size_t align) {
  /* align is always a power of 2, but we do not depend on it */
  if (!align) {
    NaClLog(0,
            "sel_ldr: NaClAlignPad, align == 0, at 0x%08"NACL_PRIxS"\n",
            val);
    return 0;
  }
  val = val % align;
  if (!val) return 0;
  return align - val;
}

/*
 * unaligned little-endian load.  precondition: nbytes should never be
 * more than 8.
 */
static uint64_t NaClLoadMem(uintptr_t addr,
                            size_t    user_nbytes) {
  uint64_t      value = 0;

  CHECK(0 != user_nbytes && user_nbytes <= 8);

  do {
    value = value << 8;
    value |= ((uint8_t *) addr)[--user_nbytes];
  } while (user_nbytes > 0);
  return value;
}

#define GENERIC_LOAD(bits) \
  static uint ## bits ## _t NaClLoad ## bits(uintptr_t addr) { \
    return (uint ## bits ## _t) NaClLoadMem(addr, sizeof(uint ## bits ## _t)); \
  }

#if NACL_TARGET_SUBARCH == 32
GENERIC_LOAD(32)
#endif
GENERIC_LOAD(64)

#undef GENERIC_LOAD

/*
 * unaligned little-endian store
 */
static void NaClStoreMem(uintptr_t  addr,
                         size_t     nbytes,
                         uint64_t   value) {
  size_t i;

  CHECK(nbytes <= 8);

  for (i = 0; i < nbytes; ++i) {
    ((uint8_t *) addr)[i] = (uint8_t) value;
    value = value >> 8;
  }
}

#define GENERIC_STORE(bits) \
  static void NaClStore ## bits(uintptr_t addr, \
                                uint ## bits ## _t v) { \
    NaClStoreMem(addr, sizeof(uint ## bits ## _t), v); \
  }

GENERIC_STORE(16)
GENERIC_STORE(32)
GENERIC_STORE(64)

#undef GENERIC_STORE

struct NaClPatchInfo *NaClPatchInfoCtor(struct NaClPatchInfo *self) {
  if (NULL != self) {
    memset(self, 0, sizeof *self);
  }
  return self;
}

/*
 * This function is called by NaClLoadTrampoline and NaClLoadSpringboard to
 * patch a single memory location specified in NaClPatchInfo structure.
 */
void  NaClApplyPatchToMemory(struct NaClPatchInfo  *patch) {
  size_t    i;
  size_t    offset;
  int64_t   reloc;
  uintptr_t target_addr;

  memcpy((void *) patch->dst, (void *) patch->src, patch->nbytes);

  reloc = patch->dst - patch->src;


  for (i = 0; i < patch->num_abs64; ++i) {
    offset = patch->abs64[i].target - patch->src;
    target_addr = patch->dst + offset;
    NaClStore64(target_addr, patch->abs64[i].value);
  }

  for (i = 0; i < patch->num_abs32; ++i) {
    offset = patch->abs32[i].target - patch->src;
    target_addr = patch->dst + offset;
    NaClStore32(target_addr, (uint32_t) patch->abs32[i].value);
  }

  for (i = 0; i < patch->num_abs16; ++i) {
    offset = patch->abs16[i].target - patch->src;
    target_addr = patch->dst + offset;
    NaClStore16(target_addr, (uint16_t) patch->abs16[i].value);
  }

  for (i = 0; i < patch->num_rel64; ++i) {
    offset = patch->rel64[i] - patch->src;
    target_addr = patch->dst + offset;
    NaClStore64(target_addr, NaClLoad64(target_addr) - reloc);
  }

  /*
   * rel32 is only supported on 32-bit architectures. The range of a relative
   * relocation in untrusted space is +/- 4GB. This can be represented as
   * an unsigned 32-bit value mod 2^32, which is handy on a 32 bit system since
   * all 32-bit pointer arithmetic is implicitly mod 2^32. On a 64 bit system,
   * however, pointer arithmetic is implicitly modulo 2^64, which isn't as
   * helpful for our purposes. We could simulate the 32-bit behavior by
   * explicitly modding all relative addresses by 2^32, but that seems like an
   * expensive way to save a few bytes per reloc.
   */
#if NACL_TARGET_SUBARCH == 32
  for (i = 0; i < patch->num_rel32; ++i) {
    offset = patch->rel32[i] - patch->src;
    target_addr = patch->dst + offset;
    NaClStore32(target_addr,
      (uint32_t) NaClLoad32(target_addr) - (int32_t) reloc);
  }
#endif
}


/*
 * Install syscall trampolines at all possible well-formed entry
 * points within the trampoline pages.  Many of these syscalls will
 * correspond to unimplemented system calls and will just abort the
 * program.
 */
void  NaClLoadTrampoline(struct NaClApp *nap) {
  int         num_syscalls;
  int         i;
  uintptr_t   addr;

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32 && __PIC__
  if (!NaClMakePcrelThunk(nap)) {
    NaClLog(LOG_FATAL, "NaClMakePcrelThunk failed!\n");
  }
#endif
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
  if (!NaClMakeDispatchThunk(nap)) {
    NaClLog(LOG_FATAL, "NaClMakeDispatchThunk failed!\n");
  }
#endif
  NaClFillTrampolineRegion(nap);

  /*
   * Do not bother to fill in the contents of page 0, since we make it
   * inaccessible later (see sel_addrspace.c, NaClMemoryProtection)
   * anyway to help detect NULL pointer errors, and we might as well
   * not dirty the page.
   *
   * The last syscall entry point is used for springboard code.
   */
  num_syscalls = ((NACL_TRAMPOLINE_END - NACL_SYSCALL_START_ADDR)
                  / NACL_SYSCALL_BLOCK_SIZE) - 1;

  NaClLog(2, "num_syscalls = %d (0x%x)\n", num_syscalls, num_syscalls);

#if defined(NACL_TARGET_ARM_THUMB2_MODE)
  CHECK(0 != ((nap->user_entry_pt | nap->initial_entry_pt) & 0x1));
  /*
   * Thumb trampolines start 2 bytes before the aligned syscall address used
   * by ordinary ARM.  We initialize this by adding 0xe to the start address
   * of each trampoline.  Because the last start address would actually start
   * into user code above, this allows one fewer trampolines than in ARM.
   */
  for (i = 0, addr = nap->mem_start + NACL_SYSCALL_START_ADDR + 0xe;
       i < num_syscalls - 1;
       ++i, addr += NACL_SYSCALL_BLOCK_SIZE) {
    NaClPatchOneTrampoline(nap, addr);
  }
#else
  for (i = 0, addr = nap->mem_start + NACL_SYSCALL_START_ADDR;
       i < num_syscalls;
       ++i, addr += NACL_SYSCALL_BLOCK_SIZE) {
    NaClPatchOneTrampoline(nap, addr);
  }
#endif
}

void  NaClMemRegionPrinter(void                   *state,
                           struct NaClVmmapEntry  *entry) {
  struct Gio *gp = (struct Gio *) state;

  gprintf(gp, "\nPage   %"NACL_PRIdPTR" (0x%"NACL_PRIxPTR")\n",
          entry->page_num, entry->page_num);
  gprintf(gp,   "npages %"NACL_PRIdS" (0x%"NACL_PRIxS")\n", entry->npages,
          entry->npages);
  gprintf(gp,   "start vaddr 0x%"NACL_PRIxPTR"\n",
          entry->page_num << NACL_PAGESHIFT);
  gprintf(gp,   "end vaddr   0x%"NACL_PRIxPTR"\n",
          (entry->page_num + entry->npages) << NACL_PAGESHIFT);
  gprintf(gp,   "prot   0x%08x\n", entry->prot);
  gprintf(gp,   "%ssrc/backed by a file\n",
          (NULL == entry->nmop) ? "not " : "");
}

void  NaClAppPrintDetails(struct NaClApp  *nap,
                          struct Gio      *gp) {
  NaClXMutexLock(&nap->mu);
  gprintf(gp,
          "NaClAppPrintDetails((struct NaClApp *) 0x%08"NACL_PRIxPTR","
          "(struct Gio *) 0x%08"NACL_PRIxPTR")\n", (uintptr_t) nap,
          (uintptr_t) gp);
  gprintf(gp, "addr space size:  2**%"NACL_PRId32"\n", nap->addr_bits);
  gprintf(gp, "stack size:       0x%08"NACL_PRIx32"\n", nap->stack_size);

  gprintf(gp, "mem start addr:   0x%08"NACL_PRIxPTR"\n", nap->mem_start);
  /*           123456789012345678901234567890 */

  gprintf(gp, "static_text_end:   0x%08"NACL_PRIxPTR"\n", nap->static_text_end);
  gprintf(gp, "end-of-text:       0x%08"NACL_PRIxPTR"\n",
          NaClEndOfStaticText(nap));
  gprintf(gp, "rodata:            0x%08"NACL_PRIxPTR"\n", nap->rodata_start);
  gprintf(gp, "data:              0x%08"NACL_PRIxPTR"\n", nap->data_start);
  gprintf(gp, "data_end:          0x%08"NACL_PRIxPTR"\n", nap->data_end);
  gprintf(gp, "break_addr:        0x%08"NACL_PRIxPTR"\n", nap->break_addr);

  gprintf(gp, "ELF initial entry point:  0x%08x\n", nap->initial_entry_pt);
  gprintf(gp, "ELF user entry point:  0x%08x\n", nap->user_entry_pt);
  gprintf(gp, "memory map:\n");
  NaClVmmapVisit(&nap->mem_map,
                 NaClMemRegionPrinter,
                 gp);
  NaClXMutexUnlock(&nap->mu);
}

struct NaClDesc *NaClGetDescMu(struct NaClApp *nap,
                               int            d) {
  struct NaClDesc *result;

  result = (struct NaClDesc *) DynArrayGet(&nap->desc_tbl, d);
  if (NULL != result) {
    NaClDescRef(result);
  }

  return result;
}

void NaClSetDescMu(struct NaClApp   *nap,
                   int              d,
                   struct NaClDesc  *ndp) {
  struct NaClDesc *result;

  result = (struct NaClDesc *) DynArrayGet(&nap->desc_tbl, d);
  NaClDescSafeUnref(result);

  if (!DynArraySet(&nap->desc_tbl, d, ndp)) {
    NaClLog(LOG_FATAL,
            "NaClSetDesc: could not set descriptor %d to 0x%08"
            NACL_PRIxPTR"\n",
            d,
            (uintptr_t) ndp);
  }
}

int32_t NaClSetAvailMu(struct NaClApp  *nap,
                       struct NaClDesc *ndp) {
  size_t pos;

  pos = DynArrayFirstAvail(&nap->desc_tbl);

  if (pos > INT32_MAX) {
    NaClLog(LOG_FATAL,
            ("NaClSetAvailMu: DynArrayFirstAvail returned a value"
             " that is greather than 2**31-1.\n"));
  }

  NaClSetDescMu(nap, (int) pos, ndp);

  return (int32_t) pos;
}

struct NaClDesc *NaClGetDesc(struct NaClApp *nap,
                             int            d) {
  struct NaClDesc *res;

  NaClXMutexLock(&nap->desc_mu);
  res = NaClGetDescMu(nap, d);
  NaClXMutexUnlock(&nap->desc_mu);
  return res;
}

void NaClSetDesc(struct NaClApp   *nap,
                 int              d,
                 struct NaClDesc  *ndp) {
  NaClXMutexLock(&nap->desc_mu);
  NaClSetDescMu(nap, d, ndp);
  NaClXMutexUnlock(&nap->desc_mu);
}

int32_t NaClSetAvail(struct NaClApp  *nap,
                     struct NaClDesc *ndp) {
  int32_t pos;

  NaClXMutexLock(&nap->desc_mu);
  pos = NaClSetAvailMu(nap, ndp);
  NaClXMutexUnlock(&nap->desc_mu);

  return pos;
}

int NaClAddThreadMu(struct NaClApp        *nap,
                    struct NaClAppThread  *natp) {
  size_t pos;

  pos = DynArrayFirstAvail(&nap->threads);

  if (!DynArraySet(&nap->threads, pos, natp)) {
    NaClLog(LOG_FATAL,
            "NaClAddThreadMu: DynArraySet at position %"NACL_PRIuS" failed\n",
            pos);
  }
  ++nap->num_threads;
  return (int) pos;
}

int NaClAddThread(struct NaClApp        *nap,
                  struct NaClAppThread  *natp) {
  int pos;

  NaClXMutexLock(&nap->threads_mu);
  pos = NaClAddThreadMu(nap, natp);
  NaClXMutexUnlock(&nap->threads_mu);

  return pos;
}

void NaClRemoveThreadMu(struct NaClApp  *nap,
                        int             thread_num) {
  if (NULL == DynArrayGet(&nap->threads, thread_num)) {
    NaClLog(LOG_FATAL,
            "NaClRemoveThreadMu:: thread to be removed is not in the table\n");
  }
  if (nap->num_threads == 0) {
    NaClLog(LOG_FATAL,
            "NaClRemoveThreadMu:: num_threads cannot be 0!!!\n");
  }
  --nap->num_threads;
  if (!DynArraySet(&nap->threads, thread_num, (struct NaClAppThread *) NULL)) {
    NaClLog(LOG_FATAL,
            "NaClRemoveThreadMu:: DynArraySet at position %d failed\n",
            thread_num);
  }
}

void NaClRemoveThread(struct NaClApp  *nap,
                      int             thread_num) {
  NaClXMutexLock(&nap->threads_mu);
  NaClRemoveThreadMu(nap, thread_num);
  NaClXCondVarBroadcast(&nap->threads_cv);
  NaClXMutexUnlock(&nap->threads_mu);
}

struct NaClAppThread *NaClGetThreadMu(struct NaClApp  *nap,
                                      int             thread_num) {
  return (struct NaClAppThread *) DynArrayGet(&nap->threads, thread_num);
}

void NaClAddHostDescriptor(struct NaClApp *nap,
                           int            host_os_desc,
                           int            flag,
                           int            nacl_desc) {
  struct NaClDescIoDesc *dp;

  NaClLog(4,
          "NaClAddHostDescriptor: host %d as nacl desc %d, flag 0x%x\n",
          host_os_desc,
          nacl_desc,
          flag);
  dp = NaClDescIoDescMake(NaClHostDescPosixMake(host_os_desc, flag));
  if (NULL == dp) {
    NaClLog(LOG_FATAL, "NaClAddHostDescriptor: NaClDescIoDescMake failed\n");
  }
  NaClSetDesc(nap, nacl_desc, (struct NaClDesc *) dp);
}

void NaClAddImcHandle(struct NaClApp  *nap,
                      NaClHandle      h,
                      int             nacl_desc) {
  struct NaClDescImcDesc  *dp;

  NaClLog(4,
          ("NaClAddImcHandle: importing NaClHandle %"NACL_PRIxPTR
           " as nacl desc %d\n"),
          (uintptr_t) h,
          nacl_desc);
  dp = (struct NaClDescImcDesc *) malloc(sizeof *dp);
  if (NULL == dp) {
    NaClLog(LOG_FATAL, "NaClAddImcHandle: no memory\n");
  }
  if (!NaClDescImcDescCtor(dp, h)) {
    NaClLog(LOG_FATAL, ("NaClAddImcHandle: cannot construct"
                        " IMC descriptor object\n"));
  }
  NaClSetDesc(nap, nacl_desc, (struct NaClDesc *) dp);
}

/*
 * Process default descriptor inheritance.  This means dup'ing
 * descriptors 0-2 and making them available to the NaCl App.
 *
 * When standard input is inherited, this could result in a NaCl
 * module competing for input from the terminal; for graphical /
 * browser plugin environments, this never is allowed to happen, and
 * having this is useful for debugging, and for potential standalone
 * text-mode applications of NaCl.
 *
 * TODO(bsy): consider whether default inheritance should occur only
 * in debug mode.
 */
void NaClAppInitialDescriptorHookup(struct NaClApp  *nap) {
  static struct {
    int         d;
    char const  *env_name;
    int         flags;
    int         mode;
    int         nacl_flags;
  } redir_control[] = {
    { 0, "NACL_EXE_STDIN",  O_RDONLY, 0,
      NACL_ABI_O_RDONLY, },
    { 1, "NACL_EXE_STDOUT", O_WRONLY | O_APPEND | O_CREAT, 0777,
      NACL_ABI_O_WRONLY | NACL_ABI_O_APPEND, },
    { 2, "NACL_EXE_STDERR", O_WRONLY | O_APPEND | O_CREAT, 0777,
      NACL_ABI_O_WRONLY | NACL_ABI_O_APPEND, },
  };

  size_t  ix;
  char    *env;
  int     d;

  for (ix = 0; ix < NACL_ARRAY_SIZE(redir_control); ++ix) {
    d = -1;
    if (NULL != (env = getenv(redir_control[ix].env_name))) {
      d = open(env, redir_control[ix].flags, redir_control[ix].mode);
      /* may return -1, esp sandbox */
    }
    if (-1 == d) {
      d = DUP(redir_control[ix].d);
    }
    NaClAddHostDescriptor(nap, d, redir_control[ix].nacl_flags, (int) ix);
  }
}

void NaClAppVmmapUpdate(struct NaClApp    *nap,
                        uintptr_t         page_num,
                        size_t            npages,
                        int               prot,
                        struct NaClMemObj *nmop,
                        int               remove) {
  NaClXMutexLock(&nap->mu);
  NaClVmmapUpdate(&nap->mem_map,
                  page_num,
                  npages,
                  prot,
                  nmop,
                  remove);
  NaClXMutexUnlock(&nap->mu);
}

uintptr_t NaClAppVmmapFindSpace(struct NaClApp  *nap,
                                int             num_pages) {
  uintptr_t rv;

  NaClXMutexLock(&nap->mu);
  rv = NaClVmmapFindSpace(&nap->mem_map,
                          num_pages);
  NaClXMutexUnlock(&nap->mu);
  return rv;
}

uintptr_t NaClAppVmmapFindMapSpace(struct NaClApp *nap,
                                   int            num_pages) {
  uintptr_t rv;

  NaClXMutexLock(&nap->mu);
  rv = NaClVmmapFindMapSpace(&nap->mem_map,
                             num_pages);
  NaClXMutexUnlock(&nap->mu);
  return rv;
}

NaClErrorCode NaClWaitForLoadModuleStatus(struct NaClApp *nap) {
  NaClErrorCode status;

  NaClXMutexLock(&nap->mu);
  while (LOAD_STATUS_UNKNOWN == (status = nap->module_load_status)) {
    NaClXCondVarWait(&nap->cv, &nap->mu);
  }
  NaClXMutexUnlock(&nap->mu);
  return status;
}

NaClErrorCode NaClWaitForStartModuleCommand(struct NaClApp *nap) {
  NaClErrorCode status;

  NaClLog(4, "NaClWaitForStartModuleCommand started\n");
  NaClXMutexLock(&nap->mu);
  while (!nap->module_may_start) {
    NaClXCondVarWait(&nap->cv, &nap->mu);
    NaClLog(4,
            "NaClWaitForStartModuleCommand: awaken, module_may_start %d\n",
            nap->module_may_start);
  }
  status = nap->module_load_status;
  NaClXMutexUnlock(&nap->mu);
  NaClLog(4, "NaClWaitForStartModuleCommand finished\n");

  return status;
}

void NaClVmHoleWaitToStartThread(struct NaClApp *nap) {
  NaClXMutexLock(&nap->mu);

  /* ensure no virtual memory hole may appear */
  while (nap->vm_hole_may_exist) {
    NaClXCondVarWait(&nap->cv, &nap->mu);
  }

  ++nap->threads_launching;
  NaClXMutexUnlock(&nap->mu);
  /*
   * NB: Dropped lock, so many threads launching can starve VM
   * operations.  If this becomes a problem in practice, we can use a
   * reader/writer lock so that a waiting writer will block new
   * readers.
   */
}

void NaClVmHoleThreadStackIsSafe(struct NaClApp *nap) {
  NaClXMutexLock(&nap->mu);

  if (0 == --nap->threads_launching) {
    /*
     * Wake up the threads waiting to do VM operations.
     */
    NaClXCondVarBroadcast(&nap->cv);
  }

  NaClXMutexUnlock(&nap->mu);
}
