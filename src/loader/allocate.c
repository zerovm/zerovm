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

// unmap 84gb of user space
void FreeUserSpace()
{
  munmap(R15_CONST, 2 * GUARDSIZE + FOURGIG);
}

// allocate 84 gb of user space
void SetUserSpace()
{
  int i;

  /* get 84gb at the fixed address */
  i = mmap(R15_CONST, SIZE_84GB, PROT_NONE, ABSOLUTE_MMAP, -1, (off_t)0);
  ZLOGFAIL(R15_CONST != i, errno, "cannot allocate 84gb");

  /* give advice to kernel */
  i = NaCl_madvise(R15_CONST, SIZE_84GB, MADV_DONTNEED);
  ZLOGIF(i != 0, "cannot madvise 84gb");
}

/* initialize trampoline using given thunk address */
static void SetTrampoline(uintptr_t thunk)
{
  int i;
  uint8_t pattern[] = { TRAMP_PATTERN };

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

// set trampoline [RX], heap [RW] and stack [RW]
// note: channels should be already set
void ProtectUserSpace(struct NaClApp *nap)
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

//  nap->manifest->mem_size
  start_addr = MEM_START + NACL_SYSCALL_START_ADDR;
  /*
   * The next pages up to NACL_TRAMPOLINE_END are the trampolines.
   * Immediately following that is the loaded text section.
   * These are collectively marked as PROT_READ | PROT_EXEC.
   */
  region_size = ROUNDUP_4K(nap->static_text_end - NACL_SYSCALL_START_ADDR);
  ZLOGS(LOG_INSANE, "Trampoline/text region start 0x%08x, size 0x%08x, end 0x%08x",
          start_addr, region_size, start_addr + region_size);

  err = NaCl_mprotect((void *)start_addr, region_size, PROT_READ | PROT_EXEC);
  ZLOGFAIL(0 != err, err, FAILED_MSG);

  if(0 != nap->rodata_start)
  {
    uintptr_t rodata_end;

    /*
     * TODO(mseaborn): Could reduce the number of cases by ensuring
     * that nap->data_start is always non-zero, even if
     * nap->rodata_start == nap->data_start == nap->break_addr.
     */
    if(0 != nap->data_start)
      rodata_end = nap->data_start;
    else rodata_end = nap->break_addr;

    start_addr = NaClUserToSys(nap->rodata_start);
    region_size = ROUNDUP_4K(ROUNDUP_64K(rodata_end) - NaClSysToUser(start_addr));
    ZLOGS(LOG_INSANE, "RO data region start 0x%08x, size 0x%08x, end 0x%08x",
        start_addr, region_size, start_addr + region_size);

    err = NaCl_mprotect((void *)start_addr, region_size, PROT_READ);
    ZLOGFAIL(0 != err, err, FAILED_MSG);
  }

  /*
   * data_end is max virtual addr seen, so start_addr <= data_end
   * must hold.
   */
  if(0 != nap->data_start)
  {
    start_addr = NaClUserToSys(nap->data_start);
    region_size = ROUNDUP_4K(ROUNDUP_64K(nap->data_end)
        - NaClSysToUser(start_addr));
    ZLOGS(LOG_INSANE, "RW data region start 0x%08x, size 0x%08x, end 0x%08x",
        start_addr, region_size, start_addr + region_size);

    err = NaCl_mprotect((void *)start_addr, region_size, PROT_READ | PROT_WRITE);
    ZLOGFAIL(0 != err, err, FAILED_MSG);
  }

  /* stack is read/write but not execute */
  region_size = STACK_SIZE;
  start_addr = NaClUserToSys(ROUNDUP_64K(((uintptr_t) 1U << ADDR_BITS) - STACK_SIZE));
  ZLOGS(LOG_INSANE, "RW stack region start 0x%08x, size 0x%08lx, end 0x%08x",
          start_addr, region_size, start_addr + region_size);

  err = NaCl_mprotect((void *)start_addr, STACK_SIZE, PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != err, err, FAILED_MSG);
}
