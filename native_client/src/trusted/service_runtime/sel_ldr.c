/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */
#include "native_client/src/include/portability_io.h"
#include "native_client/src/include/portability_string.h"
#include "native_client/src/include/nacl_macros.h"

#include "native_client/src/shared/gio/gio.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/shared/platform/nacl_time.h"
#include "native_client/src/shared/srpc/nacl_srpc.h"


#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_conn_cap.h"
#include "native_client/src/trusted/desc/nacl_desc_imc.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/desc/nrd_xfer.h"

#include "native_client/src/trusted/fault_injection/fault_injection.h"

#include "native_client/src/trusted/handle_pass/ldr_handle.h"

#include "native_client/src/trusted/reverse_service/reverse_control_rpc.h"

#include "native_client/src/trusted/gio/gio_nacl_desc.h"
#include "native_client/src/trusted/gio/gio_shm.h"
#include "native_client/src/trusted/service_runtime/arch/sel_ldr_arch.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/nacl_app.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_desc_effector_ldr.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_common.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_handlers.h"
#include "native_client/src/trusted/service_runtime/nacl_valgrind_hooks.h"
#include "native_client/src/trusted/service_runtime/sel_addrspace.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"

#include "native_client/src/trusted/service_runtime/name_service/default_name_service.h"
#include "native_client/src/trusted/service_runtime/name_service/name_service.h"

#include "native_client/src/trusted/service_runtime/sel_ldr_thread_interface.h"
#include "native_client/src/trusted/threading/nacl_thread_interface.h"

#include "native_client/src/trusted/service_runtime/include/sys/stat.h"
#include "native_client/src/trusted/service_runtime/include/sys/time.h"

#include "native_client/src/trusted/simple_service/nacl_simple_rservice.h"
#include "native_client/src/trusted/simple_service/nacl_simple_service.h"

#include "native_client/src/trusted/threading/nacl_thread_interface.h"

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

  nap->aux_info = NULL;

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

  nap->service_port = NULL;
  nap->service_address = NULL;
  nap->secure_service = NULL;
  nap->manifest_proxy = NULL;
  nap->kern_service = NULL;
  nap->reverse_client = NULL;
  nap->reverse_channel_initialization_state =
      NACL_REVERSE_CHANNEL_UNINITIALIZED;

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

  nap->name_service = (struct NaClNameService *) malloc(
      sizeof *nap->name_service);
  if (NULL == nap->name_service) {
    goto cleanup_cv;
  }
  if (!NaClNameServiceCtor(nap->name_service,
                           NaClAddrSpSquattingThreadIfFactoryFunction,
                           (void *) nap)) {
    free(nap->name_service);
    goto cleanup_cv;
  }
  nap->name_service_conn_cap = NaClDescRef(nap->name_service->
                                           base.base.bound_and_cap[1]);
  if (!NaClDefaultNameServiceInit(nap->name_service)) {
    goto cleanup_name_service;
  }

  nap->ignore_validator_result = 0;
  nap->skip_validator = 0;
  nap->validator_stub_out_mode = 0;

  if (!NaClMutexCtor(&nap->threads_mu)) {
    goto cleanup_name_service;
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
 cleanup_name_service:
  NaClDescUnref(nap->name_service_conn_cap);
  NaClRefCountUnref((struct NaClRefCount *) nap->name_service);
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
  gprintf(gp,   "%sshared/backed by a file\n",
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

void NaClCreateServiceSocket(struct NaClApp *nap) {
  struct NaClDesc *pair[2];

  NaClLog(3, "Entered NaClCreateServiceSocket\n");
  if (0 != NaClCommonDescMakeBoundSock(pair)) {
    NaClLog(LOG_FATAL, "Cound not create service socket\n");
  }
  NaClLog(4,
          "got bound socket at 0x%08"NACL_PRIxPTR", "
          "addr at 0x%08"NACL_PRIxPTR"\n",
          (uintptr_t) pair[0],
          (uintptr_t) pair[1]);
  NaClSetDesc(nap, NACL_SERVICE_PORT_DESCRIPTOR, pair[0]);
  NaClSetDesc(nap, NACL_SERVICE_ADDRESS_DESCRIPTOR, pair[1]);

  NaClDescSafeUnref(nap->service_port);

  nap->service_port = pair[0];
  NaClDescRef(nap->service_port);

  NaClDescSafeUnref(nap->service_address);

  nap->service_address = pair[1];
  NaClDescRef(nap->service_address);
  NaClLog(4, "Leaving NaClCreateServiceSocket\n");
}

void NaClSendServiceAddressTo(struct NaClApp  *nap,
                              int             desc) {
  struct NaClDesc             *channel;
  struct NaClImcTypedMsgHdr   hdr;
  ssize_t                     rv;

  NaClLog(4,
          "NaClSendServiceAddressTo(0x%08"NACL_PRIxPTR", %d)\n",
          (uintptr_t) nap,
          desc);

  channel = NaClGetDesc(nap, desc);
  if (NULL == channel) {
    NaClLog(LOG_FATAL,
            "NaClSendServiceAddressTo: descriptor %d not in open file table\n",
            desc);
    return;
  }
  if (NULL == nap->service_address) {
    NaClLog(LOG_FATAL,
            "NaClSendServiceAddressTo: service address not set\n");
    return;
  }
  /*
   * service_address and service_port are set together.
   */

  hdr.iov = (struct NaClImcMsgIoVec *) NULL;
  hdr.iov_length = 0;
  hdr.ndescv = &nap->service_address;
  hdr.ndesc_length = 1;

  rv = NaClImcSendTypedMessage(channel, &hdr, 0);

  NaClDescUnref(channel);
  channel = NULL;

  NaClLog(1,
          "NaClSendServiceAddressTo: descriptor %d, error %"NACL_PRIdS"\n",
          desc,
          rv);
}

static void NaClSecureChannelShutdownRpc(
    struct NaClSrpcRpc      *rpc,
    struct NaClSrpcArg      **in_args,
    struct NaClSrpcArg      **out_args,
    struct NaClSrpcClosure  *done) {
  UNREFERENCED_PARAMETER(rpc);
  UNREFERENCED_PARAMETER(in_args);
  UNREFERENCED_PARAMETER(out_args);
  UNREFERENCED_PARAMETER(done);

  NaClLog(4, "NaClSecureChannelShutdownRpc (hard_shutdown), exiting\n");
  NaClExit(0);
  /* Return is never reached, so no need to invoke (*done->Run)(done). */
}

/*
 * This RPC is invoked by the plugin when the nexe is downloaded as a
 * stream and not as a file. The only arguments are a handle to a
 * shared memory object that contains the nexe.
 */
static void NaClLoadModuleRpc(struct NaClSrpcRpc      *rpc,
                              struct NaClSrpcArg      **in_args,
                              struct NaClSrpcArg      **out_args,
                              struct NaClSrpcClosure  *done) {
  struct NaClApp          *nap =
      (struct NaClApp *) rpc->channel->server_instance_data;
  struct NaClDesc         *nexe_binary = in_args[0]->u.hval;
  struct NaClGioShm       gio_shm;
  struct NaClGioNaClDesc  gio_desc;
  struct Gio              *load_src = NULL;
  struct nacl_abi_stat    stbuf;
  char                    *aux;
  int                     rval;
  NaClErrorCode           suberr = LOAD_INTERNAL;
  size_t                  rounded_size;

  UNREFERENCED_PARAMETER(out_args);

  NaClLog(4, "NaClLoadModuleRpc: entered\n");

  rpc->result = NACL_SRPC_RESULT_INTERNAL;

  aux = strdup(in_args[1]->arrays.str);
  if (NULL == aux) {
    rpc->result = NACL_SRPC_RESULT_NO_MEMORY;
    goto cleanup;
  }
  NaClLog(4, "Received aux_info: %s\n", aux);

  switch (NACL_VTBL(NaClDesc, nexe_binary)->typeTag) {
    case NACL_DESC_SHM:
      /*
       * We don't know the actual size of the nexe, but it should not
       * matter.  The shared memory object's size is rounded up to at
       * least 4K, and we can map it in with uninitialized data (should be
       * zero filled) at the end.
       */
      NaClLog(4, "NaClLoadModuleRpc: finding shm size\n");

      rval = (*NACL_VTBL(NaClDesc, nexe_binary)->
              Fstat)(nexe_binary, &stbuf);
      if (0 != rval) {
        goto cleanup;
      }

      rounded_size = (size_t) stbuf.nacl_abi_st_size;

      NaClLog(4, "NaClLoadModuleRpc: shm size 0x%"NACL_PRIxS"\n", rounded_size);

      if (!NaClGioShmCtor(&gio_shm, nexe_binary, rounded_size)) {
        rpc->result = NACL_SRPC_RESULT_NO_MEMORY;
        goto cleanup;
      }
      load_src = (struct Gio *) &gio_shm;
      break;

    case NACL_DESC_HOST_IO:
      NaClLog(4, "NaClLoadModuleRpc: creating Gio from NaClDescHostDesc\n");

      if (!NaClGioNaClDescCtor(&gio_desc, nexe_binary)) {
        rpc->result = NACL_SRPC_RESULT_NO_MEMORY;
        goto cleanup;
      }
      load_src = (struct Gio *) &gio_desc;
      break;

    case NACL_DESC_INVALID:
    case NACL_DESC_DIR:
    case NACL_DESC_CONN_CAP:
    case NACL_DESC_CONN_CAP_FD:
    case NACL_DESC_BOUND_SOCKET:
    case NACL_DESC_CONNECTED_SOCKET:
    case NACL_DESC_SYSV_SHM:
    case NACL_DESC_MUTEX:
    case NACL_DESC_CONDVAR:
    case NACL_DESC_SEMAPHORE:
    case NACL_DESC_SYNC_SOCKET:
    case NACL_DESC_TRANSFERABLE_DATA_SOCKET:
    case NACL_DESC_IMC_SOCKET:
    case NACL_DESC_QUOTA:
    case NACL_DESC_DEVICE_RNG:
      /* Unsupported stuff */
      rpc->result = NACL_SRPC_RESULT_APP_ERROR;
      goto cleanup;
  }

  /*
   * do not use default case label, to make sure that the compiler
   * will generate a warning with -Wswitch-enum for new entries in
   * NaClDescTypeTag introduced in nacl_desc_base.h for which there is no
   * corresponding entry here.  instead, we pretend that fall-through
   * from the switch is possible.
   */
  if (NULL == load_src) {
    NaClLog(LOG_FATAL, "nexe_binary's typeTag has unsupported value: %d\n",
            NACL_VTBL(NaClDesc, nexe_binary)->typeTag);
  }

  /*
   * TODO(bsy): consider doing the processing below after sending the
   * RPC reply to increase parallelism.
   */

  NaClXMutexLock(&nap->mu);

  if (LOAD_STATUS_UNKNOWN != nap->module_load_status) {
    NaClLog(LOG_ERROR, "Repeated LoadModule RPC, or platform qual error?!?\n");
    if (LOAD_OK == nap->module_load_status) {
      NaClLog(LOG_ERROR, "LoadModule when module_load_status is LOAD_OK?!?\n");
      suberr = LOAD_DUP_LOAD_MODULE;
      nap->module_load_status = suberr;
    } else {
      suberr = nap->module_load_status;
    }
    rpc->result = NACL_SRPC_RESULT_OK;
    NaClXCondVarBroadcast(&nap->cv);
    goto cleanup_status_mu;
  }

  free(nap->aux_info);
  nap->aux_info = aux;

  suberr = NACL_FI_VAL("load_module", NaClErrorCode,
                       NaClAppLoadFile(load_src, nap));
  (*NACL_VTBL(Gio, load_src)->Close)(load_src);
  (*NACL_VTBL(Gio, load_src)->Dtor)(load_src);

  if (LOAD_OK != suberr) {
    nap->module_load_status = suberr;
    rpc->result = NACL_SRPC_RESULT_OK;
    NaClXCondVarBroadcast(&nap->cv);
  }

 cleanup_status_mu:
  NaClXMutexUnlock(&nap->mu);  /* NaClAppPrepareToLaunch takes mu */
  if (LOAD_OK != suberr) {
    goto cleanup;
  }

  /***************************************************************************
   * TODO(bsy): Remove/merge the code invoking NaClAppPrepareToLaunch
   * and NaClGdbHook below with sel_main's main function.  See comment
   * there.
   ***************************************************************************/

  /*
   * Finish setting up the NaCl App.
   */
  suberr = NaClAppPrepareToLaunch(nap);

  NaClXMutexLock(&nap->mu);

  nap->module_load_status = suberr;
  rpc->result = NACL_SRPC_RESULT_OK;

  NaClXCondVarBroadcast(&nap->cv);
  NaClXMutexUnlock(&nap->mu);

  /* Give debuggers a well known point at which xlate_base is known.  */
  NaClGdbHook(nap);

 cleanup:

  NaClDescUnref(nexe_binary);
  nexe_binary = NULL;
  (*done->Run)(done);
}

#if NACL_WINDOWS && !defined(NACL_STANDALONE)
/*
 * This RPC is invoked by the plugin as part of the initialization process and
 * provides the NaCl process with a socket address that can later be used for
 * pid to process handle mapping queries. This is required to enable IMC handle
 * passing inside the Chrome sandbox.
 */
static void NaClInitHandlePassingRpc(struct NaClSrpcRpc      *rpc,
                                     struct NaClSrpcArg      **in_args,
                                     struct NaClSrpcArg      **out_args,
                                     struct NaClSrpcClosure  *done) {
  struct NaClApp  *nap = (struct NaClApp *) rpc->channel->server_instance_data;
  NaClSrpcImcDescType handle_passing_socket_address = in_args[0]->u.hval;
  DWORD renderer_pid = in_args[1]->u.ival;
  NaClHandle renderer_handle = (NaClHandle)in_args[2]->u.ival;
  UNREFERENCED_PARAMETER(out_args);
  if (!NaClHandlePassLdrCtor(handle_passing_socket_address,
                             renderer_pid,
                             renderer_handle)) {
    rpc->result = NACL_SRPC_RESULT_APP_ERROR;
  } else {
    rpc->result = NACL_SRPC_RESULT_OK;
  }
  (*done->Run)(done);
}
#endif

NaClErrorCode NaClWaitForLoadModuleStatus(struct NaClApp *nap) {
  NaClErrorCode status;

  NaClXMutexLock(&nap->mu);
  while (LOAD_STATUS_UNKNOWN == (status = nap->module_load_status)) {
    NaClXCondVarWait(&nap->cv, &nap->mu);
  }
  NaClXMutexUnlock(&nap->mu);
  return status;
}

static void NaClSecureChannelStartModuleRpc(struct NaClSrpcRpc     *rpc,
                                            struct NaClSrpcArg     **in_args,
                                            struct NaClSrpcArg     **out_args,
                                            struct NaClSrpcClosure *done) {
  /*
   * let module start if module is okay; otherwise report error (e.g.,
   * ABI version mismatch).
   */
  struct NaClApp  *nap = (struct NaClApp *) rpc->channel->server_instance_data;
  NaClErrorCode   status;

  UNREFERENCED_PARAMETER(in_args);

  NaClLog(4,
          "NaClSecureChannelStartModuleRpc started, nap 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) nap);

  status = NaClWaitForLoadModuleStatus(nap);

  NaClLog(4, "NaClSecureChannelStartModuleRpc: load status %d\n", status);

  out_args[0]->u.ival = status;
  rpc->result = NACL_SRPC_RESULT_OK;

  NaClLog(4, "NaClSecureChannelStartModuleRpc running closure\n");
  (*done->Run)(done);
  /*
   * The RPC reply is now sent.  This has to occur before we signal
   * the main thread to possibly start, since in the case of a failure
   * the main thread may quickly exit.  If the main thread does this
   * before we sent the RPC reply, then the plugin will be left
   * without an answer.
   */

  NaClXMutexLock(&nap->mu);
  if (nap->module_may_start) {
    NaClLog(LOG_ERROR, "Duplicate StartModule RPC?!?\n");
    status = LOAD_DUP_START_MODULE;
  } else {
    nap->module_may_start = 1;
  }
  NaClLog(4, "NaClSecureChannelStartModuleRpc: broadcasting\n");
  NaClXCondVarBroadcast(&nap->cv);
  NaClXMutexUnlock(&nap->mu);

  NaClLog(4, "NaClSecureChannelStartModuleRpc exiting\n");
}

static void NaClSecureChannelLog(struct NaClSrpcRpc      *rpc,
                                 struct NaClSrpcArg      **in_args,
                                 struct NaClSrpcArg      **out_args,
                                 struct NaClSrpcClosure  *done) {
  int severity = in_args[0]->u.ival;
  char *msg = in_args[1]->arrays.str;

  UNREFERENCED_PARAMETER(out_args);

  NaClLog(5, "NaClSecureChannelLog started\n");
  NaClLog(severity, "%s\n", msg);
  NaClLog(5, "NaClSecureChannelLog finished\n");
  rpc->result = NACL_SRPC_RESULT_OK;
  (*done->Run)(done);
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

void NaClBlockIfCommandChannelExists(struct NaClApp *nap) {
  if (NULL != nap->secure_service) {
    for (;;) {
      struct nacl_abi_timespec req;
      req.tv_sec = 1000;
      req.tv_nsec = 0;
      NaClNanosleep(&req, (struct nacl_abi_timespec *) NULL);
    }
  }
}


/*
 * Secure command channels.
 */

struct NaClSecureService {
  struct NaClSimpleService      base;
  struct NaClApp                *nap;
};

struct NaClSimpleServiceVtbl const kNaClSecureServiceVtbl;


struct NaClConnectionHandler {
  struct NaClConnectionHandler  *next;

  /* used by NaClSimpleRevServiceClient's ClientCallback fn */
  void                          (*handler)(
      void                        *state,
      struct NaClThreadInterface  *tif,
      struct NaClDesc             *conn);
  void                          *state;
};

struct NaClSecureReverseClient {
  struct NaClSimpleRevClient        base;
  struct NaClApp                    *nap;

  struct NaClMutex                  mu;

  struct NaClConnectionHandler      *queue_head;
  struct NaClConnectionHandler      **queue_insert;
};

struct NaClSimpleRevClientVtbl const kNaClSecureReverseClientVtbl;


int NaClSecureServiceCtor(struct NaClSecureService          *self,
                          struct NaClSrpcHandlerDesc const  *srpc_handlers,
                          struct NaClApp                    *nap) {
  NaClLog(4,
          "Entered NaClSecureServiceCtor: self 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) self);
  if (!NaClSimpleServiceWithSocketCtor(&self->base,
                                       srpc_handlers,
                                       NaClThreadInterfaceThreadFactory,
                                       (void *) NULL,
                                       nap->service_port,
                                       nap->service_address)) {
    goto failure_simple_ctor;
  }
  self->nap = nap;

  NACL_VTBL(NaClRefCount, self) =
      (struct NaClRefCountVtbl *) &kNaClSecureServiceVtbl;
  return 1;
 failure_simple_ctor:
  return 0;
}

void NaClSecureServiceDtor(struct NaClRefCount *vself) {
  struct NaClSecureService *self = (struct NaClSecureService *) vself;

  NACL_VTBL(NaClRefCount, self) = (struct NaClRefCountVtbl const *)
      &kNaClSimpleServiceVtbl;
  (*NACL_VTBL(NaClRefCount, self)->Dtor)(vself);
}

/*
 * The first connection is performed by this callback handler.  This
 * spawns a client thread that will bootstrap the other connections by
 * stashing the connection represented by |conn| to make reverse RPCs
 * to ask the peer to connect to us.  No thread is spawned; we just
 * wrap access to the connection with a lock.
 *
 * Subsequent connection callbacks will pass the connection to the
 * actual thread that made the connection request using |conn|
 * received in the first connection.
 */
static void NaClSecureReverseClientCallback(
    void                        *state,
    struct NaClThreadInterface  *tif,
    struct NaClDesc             *new_conn) {
  struct NaClSecureReverseClient *self =
      (struct NaClSecureReverseClient *) state;
  struct NaClApp *nap = self->nap;

  UNREFERENCED_PARAMETER(tif);

  NaClLog(4, "Entered NaClSecureReverseClientCallback\n");

  NaClLog(4, " self=0x%"NACL_PRIxPTR"\n", (uintptr_t) self);
  NaClLog(4, " nap=0x%"NACL_PRIxPTR"\n", (uintptr_t) nap);
  NaClXMutexLock(&nap->mu);

  if (NACL_REVERSE_CHANNEL_INITIALIZATION_STARTED !=
      nap->reverse_channel_initialization_state) {
    /*
     * The reverse channel connection capability is used to make the
     * RPC that invokes this callback (this callback is invoked on a
     * reverse channel connect), so the plugin wants to initialize the
     * reverse channel and in particular the state must be either be
     * in-progress or finished.
     */
    NaClLog(LOG_FATAL, "Reverse channel already initialized\n");
  }
  if (!NaClSrpcClientCtor(&nap->reverse_channel, new_conn)) {
    NaClLog(LOG_FATAL, "Reverse channel SRPC Client Ctor failed\n");
  }
  nap->reverse_channel_initialization_state = NACL_REVERSE_CHANNEL_INITIALIZED;

  NaClXCondVarBroadcast(&nap->cv);
  NaClXMutexUnlock(&nap->mu);

  NaClLog(4, "Leaving NaClSecureReverseClientCallback\n");
}

/* fwd */
int NaClSecureReverseClientCtor(
    struct NaClSecureReverseClient *self,
    void                            (*client_callback)(
        void *, struct NaClThreadInterface *, struct NaClDesc *),
    void                            *state,
    struct NaClApp                  *nap);

static void NaClSecureReverseClientSetup(struct NaClSrpcRpc     *rpc,
                                         struct NaClSrpcArg     **in_args,
                                         struct NaClSrpcArg     **out_args,
                                         struct NaClSrpcClosure *done) {
  struct NaClApp                  *nap =
      (struct NaClApp *) rpc->channel->server_instance_data;
  struct NaClSecureReverseClient  *rev;

  UNREFERENCED_PARAMETER(in_args);
  NaClLog(4, "Entered NaClSecureReverseClientSetup\n");

  NaClXMutexLock(&nap->mu);
  if (NULL != nap->reverse_client) {
    NaClLog(LOG_FATAL, "Double reverse setup RPC\n");
  }
  if (NACL_REVERSE_CHANNEL_UNINITIALIZED !=
      nap->reverse_channel_initialization_state) {
    NaClLog(LOG_FATAL,
            "Reverse channel initialization state not uninitialized\n");
  }
  nap->reverse_channel_initialization_state =
      NACL_REVERSE_CHANNEL_INITIALIZATION_STARTED;
  /* the reverse connection is still coming */
  rev = (struct NaClSecureReverseClient *) malloc(sizeof *rev);
  if (NULL == rev) {
    rpc->result = NACL_SRPC_RESULT_APP_ERROR;
    goto done;
  }
  NaClLog(4,
          "NaClSecureReverseClientSetup: invoking"
          " NaClSecureReverseClientCtor\n");
  if (!NaClSecureReverseClientCtor(rev,
                                   NaClSecureReverseClientCallback,
                                   (void *) rev,
                                   nap)) {
    free(rev);
    rpc->result = NACL_SRPC_RESULT_APP_ERROR;
    goto done;
  }
  nap->reverse_client = (struct NaClSecureReverseClient *) NaClRefCountRef(
      (struct NaClRefCount *) rev);
  out_args[0]->u.hval = NaClDescRef(rev->base.bound_and_cap[1]);
  rpc->result = NACL_SRPC_RESULT_OK;

  /*
   * Service thread takes the reference rev.
   */
  if (!NaClSimpleRevClientStartServiceThread(&rev->base)) {
    NaClLog(LOG_FATAL, "Could not start reverse service thread\n");
  }

done:
  NaClXMutexUnlock(&nap->mu);
  (*done->Run)(done);
  NaClLog(4, "Leaving NaClSecureReverseClientSetup\n");
}

/*
 * Only called at startup and thereafter by the reverse secure
 * channel, with |self| locked.
 */
static
int NaClSecureReverseClientInsertHandler_mu(
    struct NaClSecureReverseClient  *self,
    void                            (*h)(void *,
                                         struct NaClThreadInterface *,
                                         struct NaClDesc *),
    void                            *d) {
  struct NaClConnectionHandler *entry;

  NaClLog(4,
          ("NaClSecureReverseClientInsertHandler_mu: h 0x%"NACL_PRIxPTR","
           " d 0x%"NACL_PRIxPTR"\n"),
          (uintptr_t) h,
          (uintptr_t) d);
  entry = (struct NaClConnectionHandler *) malloc(sizeof *entry);
  if (NULL == entry) {
    return 0;
  }
  entry->handler = h;
  entry->state = d;
  entry->next = (struct NaClConnectionHandler *) NULL;
  *self->queue_insert = entry;
  self->queue_insert = &entry->next;

  return 1;
}

/*
 * Caller must set up handler before issuing connection request RPC on
 * nap->reverse_channel, since otherwise the connection handler queue
 * may be empty and the connect code would abort.  Because the connect
 * doesn't wait for a handler, we don't need a condvar.
 *
 * We do not need to serialize on the handlers, since the
 * RPC-server/IMC-client implementation should not distinguish one
 * connection from another: it is okay for two handlers to be
 * inserted, and two connection request RPCs to be preformed
 * (sequentially, since they are over a single channel), and have the
 * server side spawn threads that asynchronously connect twice, in the
 * "incorrect" order, etc.
 */
int NaClSecureReverseClientInsertHandler(
    struct NaClSecureReverseClient *self,
    void                            (*handler)(
        void                        *handlr_state,
        struct NaClThreadInterface  *thread_if,
        struct NaClDesc             *new_conn),
    void                            *handler_state) {
  int retval;

  NaClXMutexLock(&self->mu);
  retval = NaClSecureReverseClientInsertHandler_mu(self,
                                                   handler, handler_state);
  NaClXMutexUnlock(&self->mu);
  return retval;
}

static
struct NaClConnectionHandler *NaClSecureReverseClientPopHandler(
    struct NaClSecureReverseClient *self) {
  struct NaClConnectionHandler *head;

  NaClLog(4, "Entered NaClSecureReverseClientPopHandler, acquiring lock\n");
  NaClXMutexLock(&self->mu);
  NaClLog(4, "NaClSecureReverseClientPopHandler, got lock\n");
  head = self->queue_head;
  if (NULL == head) {
    NaClLog(LOG_FATAL,
            "NaclSecureReverseClientPopHandler:  empty handler queue\n");
  }
  if (NULL == (self->queue_head = head->next)) {
    NaClLog(4, "NaClSecureReverseClientPopHandler, last elt patch up\n");
    self->queue_insert = &self->queue_head;
  }
  NaClLog(4, "NaClSecureReverseClientPopHandler, unlocking\n");
  NaClXMutexUnlock(&self->mu);

  head->next = NULL;
  NaClLog(4,
          ("Leaving NaClSecureReverseClientPopHandler:"
           " returning %"NACL_PRIxPTR"\n"),
          (uintptr_t) head);
  return head;
}

static
void NaClSecureReverseClientInternalCallback(
    void                        *state,
    struct NaClThreadInterface  *tif,
    struct NaClDesc             *conn) {
  struct NaClSecureReverseClient *self =
      (struct NaClSecureReverseClient *) state;
  struct NaClConnectionHandler *hand_ptr;

  UNREFERENCED_PARAMETER(tif);
  NaClLog(4, "Entered NaClSecureReverseClientInternalCallback\n");
  hand_ptr = NaClSecureReverseClientPopHandler(self);
  NaClLog(4, " got callback object %"NACL_PRIxPTR"\n", (uintptr_t) hand_ptr);
  NaClLog(4,
          " callback:0x%"NACL_PRIxPTR"(0x%"NACL_PRIxPTR",0x%"NACL_PRIxPTR")\n",
          (uintptr_t) hand_ptr->handler,
          (uintptr_t) hand_ptr->state,
          (uintptr_t) conn);
  (*hand_ptr->handler)(hand_ptr->state, tif, conn);
  NaClLog(4, "NaClSecureReverseClientInternalCallback: freeing memory\n");
  free(hand_ptr);
  NaClLog(4, "Leaving NaClSecureReverseClientInternalCallback\n");
}

/*
 * Require an initial connection handler in the Ctor, so that it's
 * obvious that a reverse client needs to accept an IMC connection
 * from the server to get things bootstrapped.
 */
int NaClSecureReverseClientCtor(
    struct NaClSecureReverseClient  *self,
    void                            (*client_callback)(
        void *, struct NaClThreadInterface*, struct NaClDesc *),
    void                            *state,
    struct NaClApp                  *nap) {
  NaClLog(4,
          ("Entered NaClSecureReverseClientCtor, self 0x%"NACL_PRIxPTR","
           " nap 0x%"NACL_PRIxPTR"\n"),
          (uintptr_t) self,
          (uintptr_t) nap);
  if (!NaClSimpleRevClientCtor(&self->base,
                               NaClSecureReverseClientInternalCallback,
                               (void *) self,
                               NaClThreadInterfaceThreadFactory,
                               (void *) NULL)) {
    goto failure_simple_ctor;
  }
  NaClLog(4, "NaClSecureReverseClientCtor: Mutex\n");
  if (!NaClMutexCtor(&self->mu)) {
    goto failure_mutex_ctor;
  }
  self->nap = nap;
  self->queue_head = (struct NaClConnectionHandler *) NULL;
  self->queue_insert = &self->queue_head;

  NACL_VTBL(NaClRefCount, self) =
      (struct NaClRefCountVtbl *) &kNaClSecureReverseClientVtbl;

  NaClLog(4, "NaClSecureReverseClientCtor: InsertHandler\n");
  if (!NaClSecureReverseClientInsertHandler(self,
                                            client_callback,
                                            state)) {
    goto failure_handler_insert;
  }

  NaClLog(4, "Leaving NaClSecureReverseClientCtor\n");
  return 1;

 failure_handler_insert:
  NaClLog(4, "NaClSecureReverseClientCtor: InsertHandler failed\n");
  NACL_VTBL(NaClRefCount, self) =
      (struct NaClRefCountVtbl *) &kNaClSimpleRevClientVtbl;

  self->nap = NULL;
  self->queue_insert = (struct NaClConnectionHandler **) NULL;
  NaClMutexDtor(&self->mu);

 failure_mutex_ctor:
  NaClLog(4, "NaClSecureReverseClientCtor: Mutex failed\n");
  (*NACL_VTBL(NaClRefCount, self)->Dtor)((struct NaClRefCount *) self);
 failure_simple_ctor:
  NaClLog(4, "Leaving NaClSecureReverseClientCtor\n");
  return 0;
}

void NaClSecureReverseClientDtor(struct NaClRefCount *vself) {
  struct NaClSecureReverseClient *self =
      (struct NaClSecureReverseClient *) vself;

  struct NaClConnectionHandler  *entry;
  struct NaClConnectionHandler  *next;

  for (entry = self->queue_head; NULL != entry; entry = next) {
    next = entry->next;
    free(entry);
  }
  NaClMutexDtor(&self->mu);

  NACL_VTBL(NaClRefCount, self) = (struct NaClRefCountVtbl const *)
      &kNaClSimpleRevClientVtbl;
  (*NACL_VTBL(NaClRefCount, self)->Dtor)(vself);
}

int NaClSecureServiceConnectionFactory(
    struct NaClSimpleService            *vself,
    struct NaClDesc                     *conn,
    struct NaClSimpleServiceConnection  **out) {
  struct NaClSecureService *self =
      (struct NaClSecureService *) vself;

  /* our instance_data is not connection specific */
  return NaClSimpleServiceConnectionFactoryWithInstanceData(
      vself, conn, (void *) self->nap, out);
}

int NaClSecureServiceAcceptAndSpawnHandler(struct NaClSimpleService *vself) {
  int rv;

  NaClLog(4,
          "NaClSecureServiceAcceptAndSpawnHandler: invoking base class vfn\n");
  rv = (*kNaClSimpleServiceVtbl.AcceptAndSpawnHandler)(vself);
  if (0 != rv) {
    NaClLog(LOG_FATAL,
            "Secure channel AcceptAndSpawnHandler returned %d\n",
            rv);
  }
  NaClThreadExit(0);
  /*
   * NOTREACHED The port is now to be used by untrusted code: all
   * subsequent connections are handled there.
   */
  return rv;
}

void NaClSecureServiceRpcHandler(struct NaClSimpleService           *vself,
                                 struct NaClSimpleServiceConnection *vconn) {

  NaClLog(4, "NaClSecureChannelThread started\n");
  (*kNaClSimpleServiceVtbl.RpcHandler)(vself, vconn);
  NaClLog(4, "NaClSecureChannelThread: channel closed, exiting.\n");
  NaClExit(0);
}

struct NaClSimpleServiceVtbl const kNaClSecureServiceVtbl = {
  {
    NaClSecureServiceDtor,
  },
  NaClSecureServiceConnectionFactory,
  NaClSimpleServiceAcceptConnection,
  NaClSecureServiceAcceptAndSpawnHandler,
  NaClSecureServiceRpcHandler,
};

struct NaClSimpleRevClientVtbl const kNaClSecureReverseClientVtbl = {
  {
    NaClSecureReverseClientDtor,
  },
};


static void WINAPI ReverseSetupThread(void *thread_state) {
  struct NaClApp *nap = (struct NaClApp *) thread_state;
  NaClSrpcError rpc_result;

  /* wait for reverse connection */
  NaClLog(1, "ReverseSetupThread\n");
  NaClXMutexLock(&nap->mu);
  while (NACL_REVERSE_CHANNEL_INITIALIZED !=
         nap->reverse_channel_initialization_state) {
    NaClLog(1, "ReverseSetupThread waiting\n");
    NaClXCondVarWait(&nap->cv, &nap->mu);
  }
  NaClLog(1, "ReverseSetupThread initialized, making rpc\n");
  rpc_result = NaClSrpcInvokeBySignature(&nap->reverse_channel,
                                         NACL_REVERSE_CONTROL_TEST,
                                         "Hello world");
  if (NACL_SRPC_RESULT_OK != rpc_result) {
    NaClLog(LOG_ERROR,
            "ReverseSetupThread, test:s: rpc_result %d\n",
            rpc_result);
  }
  NaClLog(4,
          "Hello world should be logged browser side, rpc_result %d.\n",
          rpc_result);
  rpc_result = NaClSrpcInvokeBySignature(&nap->reverse_channel,
                                         NACL_REVERSE_CONTROL_LOG,
                                         ("Log message that should show up"
                                          " on the JavaScript console"));
  NaClLog(4, "revlog returned %d.\n", rpc_result);
  if (NACL_SRPC_RESULT_OK != rpc_result) {
    NaClLog(LOG_ERROR,
            "ReverseSetupThread, revlog:s: rpc_result %d\n",
            rpc_result);
  }
  NaClXMutexUnlock(&nap->mu);

  NaClThreadDtor(&nap->reverse_setup_thread);
}


void NaClSecureCommandChannel(struct NaClApp *nap) {
  struct NaClSecureService *secure_command_server;

  static struct NaClSrpcHandlerDesc const secure_handlers[] = {
    { "hard_shutdown::", NaClSecureChannelShutdownRpc, },
    { "start_module::i", NaClSecureChannelStartModuleRpc, },
    { "log:is:", NaClSecureChannelLog, },
    { "load_module:hs:", NaClLoadModuleRpc, },
#if NACL_WINDOWS && !defined(NACL_STANDALONE)
    { "init_handle_passing:hii:", NaClInitHandlePassingRpc, },
#endif
    { "reverse_setup::h", NaClSecureReverseClientSetup, },
    /* add additional calls here.  upcall set up?  start module signal? */
    { (char const *) NULL, (NaClSrpcMethod) NULL, },
  };

  NaClLog(4, "Entered NaClSecureCommandChannel\n");

  secure_command_server = (struct NaClSecureService *) malloc(
      sizeof *secure_command_server);
  if (NULL == secure_command_server) {
    NaClLog(LOG_FATAL, "Out of memory for secure command channel\n");
  }
  if (!NaClSecureServiceCtor(secure_command_server, secure_handlers, nap)) {
    NaClLog(LOG_FATAL, "NaClSecureServiceCtor failed\n");
  }
  nap->secure_service = secure_command_server;

  NaClLog(4, "NaClSecureCommandChannel: starting service thread\n");
  if (!NaClSimpleServiceStartServiceThread((struct NaClSimpleService *)
                                           secure_command_server)) {
    NaClLog(LOG_FATAL,
            "Could not start secure command channel service thread\n");
  }

  if (!NaClThreadCtor(&nap->reverse_setup_thread, ReverseSetupThread, nap,
                      NACL_KERN_STACK_SIZE)) {
    NaClLog(LOG_FATAL,
            "Could not start reverse setup thread\n");
  }

  NaClLog(4, "Leaving NaClSecureCommandChannel\n");
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

#ifdef __GNUC__

/*
 * GDB's canonical overlay managment routine.
 * We need its symbol in the symbol table so don't inline it.
 * TODO(dje): add some explanation for the non-GDB person.
 */

static void __attribute__ ((noinline)) _ovly_debug_event (void) {
  /*
   * The asm volatile is here as instructed by the GCC docs.
   * It's not enough to declare a function noinline.
   * GCC will still look inside the function to see if it's worth calling.
   */
  __asm__ volatile ("");
}

#endif

static void StopForDebuggerInit (uintptr_t mem_start) {
  /* Put xlate_base in a place where gdb can find it.  */
  nacl_global_xlate_base = mem_start;

  NaClSandboxMemoryStartForValgrind(mem_start);

#ifdef __GNUC__
  _ovly_debug_event ();
#endif
}

void NaClGdbHook(struct NaClApp const *nap) {
  StopForDebuggerInit(nap->mem_start);
}
