/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/portability_string.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_asm_symbols.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"
#include "native_client/src/trusted/service_runtime/springboard.h"
#include "native_client/src/trusted/service_runtime/arch/x86/sel_ldr_x86.h"
#include "native_client/src/trusted/service_runtime/arch/x86_64/tramp_64.h"

int NaClMakeDispatchThunk(struct NaClApp *nap) {
  int                   retval = 0;  /* fail */
  int                   error;
  void                  *thunk_addr = NULL;
  struct NaClPatchInfo  patch_info;
  struct NaClPatch      jmp_target;

  NaClLog(LOG_WARNING, "Entered NaClMakeDispatchThunk\n");
  if (0 != nap->dispatch_thunk) {
    NaClLog(LOG_ERROR, " dispatch_thunk already initialized!\n");
    return 1;
  }

  if (0 != (error = NaCl_page_alloc_randomized(&thunk_addr,
                                               NACL_MAP_PAGESIZE))) {
    NaClLog(LOG_INFO,
            "NaClMakeDispatchThunk::NaCl_page_alloc failed, errno %d\n",
            -error);
    retval = 0;
    goto cleanup;
  }
  NaClLog(LOG_INFO, "NaClMakeDispatchThunk: got addr 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) thunk_addr);

  if (0 != (error = NaCl_mprotect(thunk_addr,
                                  NACL_MAP_PAGESIZE,
                                  PROT_READ | PROT_WRITE))) {
    NaClLog(LOG_INFO,
            "NaClMakeDispatchThunk::NaCl_mprotect r/w failed, errno %d\n",
            -error);
    retval = 0;
    goto cleanup;
  }
  NaClFillMemoryRegionWithHalt(thunk_addr, NACL_MAP_PAGESIZE);

  jmp_target.target = (((uintptr_t) &NaClDispatchThunk_jmp_target)
                       - sizeof(uintptr_t));
  jmp_target.value = (uintptr_t) NaClSyscallSeg;

  NaClPatchInfoCtor(&patch_info);
  patch_info.abs64 = &jmp_target;
  patch_info.num_abs64 = 1;

  patch_info.dst = (uintptr_t) thunk_addr;
  patch_info.src = (uintptr_t) &NaClDispatchThunk;
  patch_info.nbytes = ((uintptr_t) &NaClDispatchThunkEnd
                       - (uintptr_t) &NaClDispatchThunk);
  NaClApplyPatchToMemory(&patch_info);

  if (0 != (error = NaCl_mprotect(thunk_addr,
                                  NACL_MAP_PAGESIZE,
                                  PROT_EXEC|PROT_READ))) {
    NaClLog(LOG_INFO,
            "NaClMakeDispatchThunk::NaCl_mprotect r/x failed, errno %d\n",
            -error);
    retval = 0;
    goto cleanup;
  }
  retval = 1;
 cleanup:
  if (0 == retval) {
    if (NULL != thunk_addr) {
      NaCl_page_free(thunk_addr, NACL_MAP_PAGESIZE);
      thunk_addr = NULL;
    }
  } else {
    nap->dispatch_thunk = (uintptr_t) thunk_addr;
  }
  return retval;
}

/*
 * Install a syscall trampoline at target_addr.  NB: Thread-safe.
 */
void  NaClPatchOneTrampoline(struct NaClApp *nap,
                             uintptr_t      target_addr) {
  struct NaClPatchInfo  patch_info;
  struct NaClPatch      call_target;
  uintptr_t             call_target_addr;

  call_target_addr = nap->dispatch_thunk;

  NaClLog(4, "call_target_addr = 0x%"NACL_PRIxPTR"\n", call_target_addr);
  CHECK(0 != call_target_addr);
  call_target.target = (((uintptr_t) &NaCl_trampoline_call_target)
                        - sizeof(uintptr_t));
  call_target.value = call_target_addr;

  NaClPatchInfoCtor(&patch_info);

  patch_info.abs64 = &call_target;
  patch_info.num_abs64 = 1;

  patch_info.dst = target_addr;
  patch_info.src = (uintptr_t) &NaCl_trampoline_code;
  patch_info.nbytes = ((uintptr_t) &NaCl_trampoline_code_end
                       - (uintptr_t) &NaCl_trampoline_code);

  NaClApplyPatchToMemory(&patch_info);
}

void NaClFillMemoryRegionWithHalt(void *start, size_t size) {
  CHECK(!(size % NACL_HALT_LEN));
  /* Tell valgrind that this memory is accessible and undefined */
  NACL_MAKE_MEM_UNDEFINED(start, size);
  memset(start, NACL_HALT_OPCODE, size);
}

void NaClFillTrampolineRegion(struct NaClApp *nap) {
  NaClFillMemoryRegionWithHalt(
      (void *) (nap->mem_start + NACL_TRAMPOLINE_START),
      NACL_TRAMPOLINE_SIZE);
}

void NaClLoadSpringboard(struct NaClApp  *nap) {
  /*
   * There is no springboard for x86-64.
   */
  UNREFERENCED_PARAMETER(nap);
}
