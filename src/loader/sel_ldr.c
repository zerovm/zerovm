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
#include <assert.h>
#include "src/loader/sel_ldr.h"
#include "src/loader/sel_addrspace.h"
#include "src/platform/sel_memory.h"
#include "src/loader/tramp.h"

#define THUNK_ADDR ((void*)0x5AFECA110000)

/*
 * target is an absolute address in the source region.  the patch code
 * will figure out the corresponding address in the destination region
 * and modify as appropriate.  this makes it easier to specify, since
 * the target is typically the address of some symbol from the source
 * template.
 */
struct Patch {
  uintptr_t    target;
  uint64_t     value;
};

struct PatchInfo {
  uintptr_t    dst;
  uintptr_t    src;
  size_t       nbytes;

  struct Patch *abs16;
  size_t       num_abs16;

  struct Patch *abs32;
  size_t       num_abs32;

  struct Patch *abs64;
  size_t       num_abs64;
};

extern int SyscallSeg(); /* d'b: defined in to_trap.S */
static uintptr_t dispatch_thunk = 0;

static struct PatchInfo *PatchInfoCtor(struct PatchInfo *self)
{
  if(NULL != self)
  {
    memset(self, 0, sizeof *self);
  }
  return self;
}

/* unaligned little-endian store */
static void StoreMem(uintptr_t addr, size_t nbytes, uint64_t value)
{
  size_t i;

  ZLOGFAIL(nbytes > 8, EFAULT, FAILED_MSG);

  for(i = 0; i < nbytes; ++i)
  {
    ((uint8_t *)addr)[i] = (uint8_t)value;
    value = value >> 8;
  }
}

#define GENERIC_STORE(bits) \
  static void Store ## bits(uintptr_t addr, uint ## bits ## _t v) { \
    StoreMem(addr, sizeof(uint ## bits ## _t), v); \
  }

GENERIC_STORE(16)
GENERIC_STORE(32)
GENERIC_STORE(64)

#undef GENERIC_STORE

/*
 * This function is called by NaClLoadTrampoline and NaClLoadSpringboard to
 * patch a single memory location specified in NaClPatchInfo structure.
 */
static void ApplyPatchToMemory(struct PatchInfo *patch)
{
  size_t i;
  size_t offset;
  uintptr_t target_addr;

  memcpy((void *) patch->dst, (void *) patch->src, patch->nbytes);

  for(i = 0; i < patch->num_abs64; ++i)
  {
    offset = patch->abs64[i].target - patch->src;
    target_addr = patch->dst + offset;
    Store64(target_addr, patch->abs64[i].value);
  }

  for(i = 0; i < patch->num_abs32; ++i)
  {
    offset = patch->abs32[i].target - patch->src;
    target_addr = patch->dst + offset;
    Store32(target_addr, (uint32_t) patch->abs32[i].value);
  }

  for(i = 0; i < patch->num_abs16; ++i)
  {
    offset = patch->abs16[i].target - patch->src;
    target_addr = patch->dst + offset;
    Store16(target_addr, (uint16_t) patch->abs16[i].value);
  }
}

static void MakeDispatchThunk()
{
  int                   error;
  void                  *thunk_addr = THUNK_ADDR;
  struct PatchInfo  patch_info;
  struct Patch      jmp_target;

  assert(dispatch_thunk == 0);

  /* d'b: replaced with not randomized version */
  error = NaCl_page_alloc_intern_flags(&thunk_addr, NACL_MAP_PAGESIZE, 0);
  ZLOGFAIL(error < 0, errno, "thunk allocation failed");

  error = NaCl_mprotect(thunk_addr, NACL_MAP_PAGESIZE, PROT_READ | PROT_WRITE);
  ZLOGFAIL(error != 0, errno, "thunk r/w protection failed");

  FillMemoryRegionWithHalt(thunk_addr, NACL_MAP_PAGESIZE);

  jmp_target.target = ((uintptr_t)&NaClDispatchThunk_jmp_target) - sizeof(uintptr_t);
  jmp_target.value = (uintptr_t) SyscallSeg;

  PatchInfoCtor(&patch_info);
  patch_info.abs64 = &jmp_target;
  patch_info.num_abs64 = 1;

  patch_info.dst = (uintptr_t) thunk_addr;
  patch_info.src = (uintptr_t) &NaClDispatchThunk;
  patch_info.nbytes = (uintptr_t)&NaClDispatchThunkEnd - (uintptr_t)&NaClDispatchThunk;
  ApplyPatchToMemory(&patch_info);

  error = NaCl_mprotect(thunk_addr, NACL_MAP_PAGESIZE, PROT_EXEC | PROT_READ);
  ZLOGFAIL(error != 0, errno, "thunk r/x protection failed");

  dispatch_thunk = (uintptr_t)thunk_addr;
}

/* Install a syscall trampoline at target_addr. NB: Thread-safe. */
static void PatchOneTrampoline(uintptr_t target_addr)
{
  struct PatchInfo patch_info;
  struct Patch call_target;
  uintptr_t call_target_addr;

  call_target_addr = dispatch_thunk;

  ZLOGS(LOG_INSANE, "call_target_addr = 0x%lx", call_target_addr);
  ZLOGFAIL(0 == call_target_addr, EFAULT, FAILED_MSG);

  call_target.target = ((uintptr_t)&NaCl_trampoline_call_target) - sizeof(uintptr_t);
  call_target.value = call_target_addr;

  PatchInfoCtor(&patch_info);

  patch_info.abs64 = &call_target;
  patch_info.num_abs64 = 1;

  patch_info.dst = target_addr;
  patch_info.src = (uintptr_t)&NaCl_trampoline_code;
  patch_info.nbytes = (uintptr_t)&NaCl_trampoline_code_end - (uintptr_t)&NaCl_trampoline_code;

  ApplyPatchToMemory(&patch_info);
}

void FreeDispatchThunk()
{
  if((void*)dispatch_thunk == NULL) return;
  NaCl_page_free((void*)dispatch_thunk, NACL_MAP_PAGESIZE);
  dispatch_thunk = (uintptr_t)NULL;
}

void FillMemoryRegionWithHalt(void *start, size_t size)
{
  ZLOGFAIL(size % NACL_HALT_LEN, EFAULT, FAILED_MSG);
  memset(start, NACL_HALT_OPCODE, size);
}

static void FillTrampolineRegion(struct NaClApp *nap)
{
  FillMemoryRegionWithHalt
    ((void *)(nap->mem_start + NACL_TRAMPOLINE_START), NACL_TRAMPOLINE_SIZE);
}

void NaClAppCtor(struct NaClApp *nap)
{
  nap->addr_bits = NACL_MAX_ADDR_BITS;

  gnap = nap;
  nacl_user = g_malloc(sizeof *nacl_user);
  nacl_sys = g_malloc(sizeof *nacl_sys);
}

void NaClAppDtor(struct NaClApp *nap)
{
  FreeAddrSpace(nap);
  g_free(nacl_sys);
  g_free(nacl_user);
  nacl_sys = NULL;
  nacl_user = NULL;
}

void LoadTrampoline(struct NaClApp *nap)
{
  int         num_syscalls;
  int         i;
  uintptr_t   addr;

  MakeDispatchThunk();
  FillTrampolineRegion(nap);

  /*
   * Do not bother to fill in the contents of page 0, since we make it
   * inaccessible later (see sel_addrspace.c, NaClMemoryProtection)
   * anyway to help detect NULL pointer errors, and we might as well
   * not dirty the page.
   *
   * The last syscall entry point is reserved (nacl springboard code)
   */
  num_syscalls = ((NACL_TRAMPOLINE_END - NACL_SYSCALL_START_ADDR) / NACL_SYSCALL_BLOCK_SIZE) - 1;
  ZLOGS(LOG_INSANE, "num_syscalls = %d (0x%x)", num_syscalls, num_syscalls);

  addr = nap->mem_start + NACL_SYSCALL_START_ADDR;
  for (i = 0; i < num_syscalls; ++i)
  {
    PatchOneTrampoline(addr);
    addr += NACL_SYSCALL_BLOCK_SIZE;
  }
}

void PrintAppDetails(struct NaClApp *nap, int verbosity)
{
  ZLOGS(verbosity, "NaClAppPrintDetails((struct NaClApp*) 0x%08lx,", (uintptr_t)nap);
  ZLOGS(verbosity, "addr space size:         2**%d", nap->addr_bits);
  ZLOGS(verbosity, "stack size:              0x%08d", STACK_SIZE);
  ZLOGS(verbosity, "mem start addr:          0x%08lx", nap->mem_start);
  ZLOGS(verbosity, "static_text_end:         0x%08lx", nap->static_text_end);
  ZLOGS(verbosity, "end-of-text:             0x%08lx", NaClEndOfStaticText(nap));
  ZLOGS(verbosity, "rodata:                  0x%08lx", nap->rodata_start);
  ZLOGS(verbosity, "data:                    0x%08lx", nap->data_start);
  ZLOGS(verbosity, "data_end:                0x%08lx", nap->data_end);
  ZLOGS(verbosity, "break_addr:              0x%08lx", nap->break_addr);
  ZLOGS(verbosity, "ELF initial entry point: 0x%08x", nap->initial_entry_pt);
  ZLOGS(verbosity, "ELF user entry point:    0x%08x", nap->user_entry_pt);
}
