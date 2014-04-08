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

#include <assert.h>
#include <sys/mman.h>
#include "src/loader/sel_ldr.h"
#include "src/platform/sel_memory.h"

/*
 * this module allocates user space. protects bumpers and NULL.
 * prepares trampoline and stack
 */
#define SIZE_84GB (2 * GUARDSIZE + FOURGIG)
#define PROXY_IDX 2
#define TRAMP_PATTERN \
  0x48, 0xb8, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, \
  0xf4, 0xf4, 0xff, 0xd0, 0xf4, 0xf4, 0xf4, 0xf4, \
  0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, \
  0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4

/* allocate 84 gb of user space */
void MakeUserSpace()
{
  int i;

  /* get 84gb at the fixed address */
  i = mmap(R15_CONST, SIZE_84GB, PROT_NONE, ABSOLUTE_MMAP, -1, (off_t)0);
  ZLOGFAIL(R15_CONST != i, errno, "cannot allocate 84gb");

  /* give advice to kernel */
  i = NaCl_madvise(R15_CONST, SIZE_84GB, MADV_DONTNEED);
  ZLOGIF(i != 0, "cannot madvise 84gb");
}

/* unmap 84gb of user space */
void FreeUserSpace()
{
  munmap(R15_CONST, 2 * GUARDSIZE + FOURGIG);
}

/* initialize trampoline using given thunk address */
static void SetTrampoline(uintptr_t thunk)
{
  int i;
  uint8_t pattern[] = { TRAMP_PATTERN };

#if 0 /* ### another way to initialize pattern */
  uint8_t pattern[32];

  memset(pattern, 0xf4, 32);
  *(uint16_t*)pattern = 0xb848;
  *(uint16_t*)(pattern + 10) = 0xd0ff;
#endif

  /* change protection of area to RW */
  i = NaCl_mprotect((void*)(MEM_START + NACL_TRAMPOLINE_START),
      NACL_TRAMPOLINE_SIZE, PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != i, ENOMEM, "cannot make trampoline writable");

  /* create trampoline call pattern */
  *(uintptr_t*)(pattern + PROXY_IDX) = thunk;

  /* populate trampoline area with it */
  for(i = 0; i < NACL_TRAMPOLINE_SIZE / ARRAY_SIZE(pattern); ++i)
    memset((char*)NACL_TRAMPOLINE_START + i * ARRAY_SIZE(pattern),
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

/*
 * initialize user memory with mandatory areas:
 * trampoline [RX], heap [RW], manifest [RX] and stack [RW]
 * note: channels should be already set
 */
void SetUserSpace(struct NaClApp *nap)
{
  uintptr_t start_addr;
  size_t    region_size;
  int       err;

  /* set trampoline */
  SetTrampoline();

  /* call function to set user manifest and heap */
  SetUserManifest();

  /* set stack */
  SetStack();
}
