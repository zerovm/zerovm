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
 * TODO(d'b): the replacement for sel_addrspace. will be activated after
 * SEL will be removed
 */

#include <assert.h>
#include <sys/mman.h>
#include "src/loader/sel_ldr.h"
#include "src/platform/sel_memory.h"

/*
 * this module allocates user space. protects bumpers and NULL.
 * prepares trampoline and stack
 */
#define SIZE_84GB (2 * GUARDSIZE + FOURGIG)
#define USER_PTR ((void*)0x440000000000)
#define TRAMP_IDX 2
#define TRAMP_PATTERN \
  0x48, 0xb8, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, \
  0xf4, 0xf4, 0xff, 0xd0, 0xf4, 0xf4, 0xf4, 0xf4, \
  0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, \
  0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4
#define PROXY_PTR ((void*)0x5AFECA110000)
#define PROXY_SIZE NACL_MAP_PAGESIZE
#define PROXY_IDX 2

extern int SyscallSeg(); /* defined in to_trap.S */

/* allocate and set call proxy */
static void MakeTrapProxy()
{
  int i;
  void *p;

  /* allocate page for proxy */
  p = mmap(PROXY_PTR, PROXY_SIZE, PROT_WRITE, ABSOLUTE_MMAP, -1, (off_t)0);
  ZLOGFAIL(PROXY_PTR != p, errno, "cannot allocate proxy");

  /* populate proxy area with halts */
  memset(PROXY_PTR, NACL_HALT_OPCODE, PROXY_SIZE);

  /* patch proxy with trap address */
  *(uintptr_t*)(PROXY_PTR + PROXY_IDX) = (uintptr_t)&SyscallSeg;

  /* change proxy protection */
  i = NaCl_mprotect(PROXY_PTR, PROXY_SIZE, PROT_READ | PROT_EXEC);
  ZLOGFAIL(0 != i, ENOMEM, "cannot set stack protection");
}

/* free call proxy */
static void FreeTrapProxy()
{
  munmap(PROXY_PTR, PROXY_SIZE);
}

void MakeUserSpace()
{
  int i;
  void *p;

  /* get 84gb at the fixed address */
  p = mmap(USER_PTR, SIZE_84GB, PROT_NONE, ABSOLUTE_MMAP, -1, (off_t)0);
  ZLOGFAIL(USER_PTR != p, errno, "cannot allocate 84gb");

  /* give advice to kernel */
  i = NaCl_madvise(USER_PTR, SIZE_84GB, MADV_DONTNEED);
  ZLOGIF(i != 0, "cannot madvise 84gb");

  MakeTrapProxy();
}

void FreeUserSpace()
{
  munmap(USER_PTR, 2 * GUARDSIZE + FOURGIG);
  FreeTrapProxy();
}

/* initialize trampoline using given thunk address */
static void SetTrampoline()
{
  int i;
  uint8_t pattern[] = { TRAMP_PATTERN };

  /* change protection of area to RW */
  i = NaCl_mprotect((void*)(MEM_START + NACL_TRAMPOLINE_START),
      NACL_TRAMPOLINE_SIZE, PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != i, ENOMEM, "cannot make trampoline writable");

  /* create trampoline call pattern */
  *(uintptr_t*)(pattern + TRAMP_IDX) = (uintptr_t)PROXY_PTR;

  /* populate trampoline area with it */
  for(i = 0; i < NACL_TRAMPOLINE_SIZE / ARRAY_SIZE(pattern); ++i)
    memcpy((char*)NACL_TRAMPOLINE_START + i * ARRAY_SIZE(pattern),
        pattern, ARRAY_SIZE(pattern));

  /* change protection of area to RX */
  i = NaCl_mprotect((void*)(MEM_START + NACL_TRAMPOLINE_START),
      NACL_TRAMPOLINE_SIZE, PROT_READ | PROT_EXEC);
  ZLOGFAIL(0 != i, ENOMEM, "cannot make trampoline executable");

  /* give kernel advice */
  i = NaCl_madvise((void*)(MEM_START + NACL_TRAMPOLINE_START),
      NACL_TRAMPOLINE_SIZE, MADV_WILLNEED);
  ZLOGIF(i != 0, "cannot madvise trampoline");
}

/* initialize stack */
static void SetStack()
{
  int i;

  /* change protection of stack to RW */
  i = NaCl_mprotect((void*)(MEM_START + FOURGIG - STACK_SIZE),
      STACK_SIZE, PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != i, ENOMEM, "cannot set stack protection");

  /* give kernel advice */
  i = NaCl_madvise((void*)(MEM_START + NACL_TRAMPOLINE_START),
      NACL_TRAMPOLINE_SIZE, MADV_NORMAL);
  ZLOGIF(i != 0, "cannot madvise stack");
}

/* set user manifest and initialize heap */
static void SetUserManifest()
{

}

void SetUserSpace(struct NaClApp *nap)
{
  SetTrampoline();
  SetUserManifest();
  SetStack();
}
