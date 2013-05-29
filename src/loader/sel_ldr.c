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
 */
#include "src/main/zlog.h"
#include "src/platform/gio.h"
#include "src/loader/sel_ldr.h"
#include "src/loader/sel_ldr_x86.h"
#include "src/loader/sel_addrspace.h"
#include "src/main/nacl_globals.h"

void NaClAppCtor(struct NaClApp *nap)
{
  nap->addr_bits = NACL_MAX_ADDR_BITS;
  nap->stack_size = NACL_DEFAULT_STACK_MAX;

  gnap = nap;
  nacl_user = g_malloc(sizeof *nacl_user);
  nacl_sys = g_malloc(sizeof *nacl_sys);
}

void NaClAppDtor(struct NaClApp *nap)
{
  NaClFreeAddrSpace(nap);
  g_free(nacl_sys);
  g_free(nacl_user);
  nacl_sys = NULL;
  nacl_user = NULL;
}

/* unaligned little-endian store */
static void NaClStoreMem(uintptr_t addr, size_t nbytes, uint64_t value)
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

void  NaClApplyPatchToMemory(struct NaClPatchInfo  *patch) {
  size_t    i;
  size_t    offset;
  uintptr_t target_addr;

  memcpy((void *) patch->dst, (void *) patch->src, patch->nbytes);

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
}

void  NaClLoadTrampoline(struct NaClApp *nap) {
  int         num_syscalls;
  int         i;
  uintptr_t   addr;

  ZLOGFAIL(!NaClMakeDispatchThunk(nap), EFAULT, FAILED_MSG);
  NaClFillTrampolineRegion(nap);

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
    NaClPatchOneTrampoline(nap, addr);
    addr += NACL_SYSCALL_BLOCK_SIZE;
  }
}

void NaClAppPrintDetails(struct NaClApp *nap, struct Gio *gp, int verbosity)
{
  ZLOGS(verbosity, "NaClAppPrintDetails((struct NaClApp *) 0x%08lx,"
      "(struct Gio *) 0x%08lx)", (uintptr_t)nap, (uintptr_t)gp);
  ZLOGS(verbosity, "addr space size:         2**%d", nap->addr_bits);
  ZLOGS(verbosity, "stack size:              0x%08d", nap->stack_size);
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
