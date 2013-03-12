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

#include <sys/mman.h>
#include "src/main/zlog.h"
#include "src/loader/sel_ldr.h"
#include "src/platform/sel_memory.h"
#include "src/loader/sel_ldr_x86.h"
#include "src/loader/tramp_64.h"

extern int NaClSyscallSeg(); /* d'b: defined in nacl_syscall_64.S */

int NaClMakeDispatchThunk(struct NaClApp *nap)
{
  int                   retval = 0;  /* fail */
  int                   error;
  void                  *thunk_addr = ((void*)0x5AFECA110000); /* d'b */
  struct NaClPatchInfo  patch_info;
  struct NaClPatch      jmp_target;

  if(0 != nap->dispatch_thunk)
  {
    ZLOG(LOG_ERROR, "dispatch_thunk already initialized!");
    return 1;
  }

  /* d'b: replaced with not randomized version */
  error = NaCl_page_alloc_intern_flags(&thunk_addr, NACL_MAP_PAGESIZE, 0);
  if(0 != (error))
  {
    ZLOG(LOG_ERROR, "NaCl_page_alloc_intern_flags failed, errno %d", -error);
    retval = 0;
    goto cleanup;
  }

  ZLOGS(LOG_DEBUG, "got addr 0x%lx", (uintptr_t) thunk_addr);
  if(0 != (error = NaCl_mprotect(thunk_addr, NACL_MAP_PAGESIZE, PROT_READ | PROT_WRITE)))
  {
    ZLOG(LOG_ERROR, "NaCl_mprotect r/w failed, errno %d", -error);
    retval = 0;
    goto cleanup;
  }

  NaClFillMemoryRegionWithHalt(thunk_addr, NACL_MAP_PAGESIZE);

  jmp_target.target = ((uintptr_t)&NaClDispatchThunk_jmp_target) - sizeof(uintptr_t);
  jmp_target.value = (uintptr_t) NaClSyscallSeg;

  NaClPatchInfoCtor(&patch_info);
  patch_info.abs64 = &jmp_target;
  patch_info.num_abs64 = 1;

  patch_info.dst = (uintptr_t) thunk_addr;
  patch_info.src = (uintptr_t) &NaClDispatchThunk;
  patch_info.nbytes = (uintptr_t)&NaClDispatchThunkEnd - (uintptr_t)&NaClDispatchThunk;
  NaClApplyPatchToMemory(&patch_info);

  if(0 != (error = NaCl_mprotect(thunk_addr, NACL_MAP_PAGESIZE, PROT_EXEC | PROT_READ)))
  {
    ZLOG(LOG_ERROR, "NaCl_mprotect r/x failed, errno %d", -error);
    retval = 0;
    goto cleanup;
  }
  retval = 1;

  cleanup:
  if(0 == retval)
  {
    if(NULL != thunk_addr)
    {
      NaCl_page_free(thunk_addr, NACL_MAP_PAGESIZE);
      thunk_addr = NULL;
    }
  }
  else
  {
    nap->dispatch_thunk = (uintptr_t)thunk_addr;
  }
  return retval;
}

void NaClPatchOneTrampoline(struct NaClApp *nap, uintptr_t target_addr)
{
  struct NaClPatchInfo patch_info;
  struct NaClPatch call_target;
  uintptr_t call_target_addr;

  call_target_addr = nap->dispatch_thunk;

  ZLOGS(LOG_INSANE, "call_target_addr = 0x%lx", call_target_addr);
  ZLOGFAIL(0 == call_target_addr, EFAULT, FAILED_MSG);

  call_target.target = ((uintptr_t)&NaCl_trampoline_call_target) - sizeof(uintptr_t);
  call_target.value = call_target_addr;

  NaClPatchInfoCtor(&patch_info);

  patch_info.abs64 = &call_target;
  patch_info.num_abs64 = 1;

  patch_info.dst = target_addr;
  patch_info.src = (uintptr_t)&NaCl_trampoline_code;
  patch_info.nbytes = (uintptr_t)&NaCl_trampoline_code_end - (uintptr_t)&NaCl_trampoline_code;

  NaClApplyPatchToMemory(&patch_info);
}

void NaClFillMemoryRegionWithHalt(void *start, size_t size)
{
  ZLOGFAIL(size % NACL_HALT_LEN, EFAULT, FAILED_MSG);
  memset(start, NACL_HALT_OPCODE, size);
}

void NaClFillTrampolineRegion(struct NaClApp *nap)
{
  NaClFillMemoryRegionWithHalt
    ((void *)(nap->mem_start + NACL_TRAMPOLINE_START), NACL_TRAMPOLINE_SIZE);
}
