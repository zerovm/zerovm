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
#include "src/loader/sel_ldr.h" // ### to remove
#include "src/loader/userspace.h"
#include "src/loader/usermap.h"
#include "src/channels/channel.h"
#include "src/channels/serializer.h"

/*
 * TODO(d'b): collect all defines in proper place under proper names
 */
#define ABSOLUTE_MMAP (MAP_ANONYMOUS | MAP_NORESERVE | MAP_PRIVATE | MAP_FIXED)
#define TRAMP_IDX 2
#define TRAMP_PATTERN \
  0x48, 0xb8, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, \
  0xf4, 0xf4, 0xff, 0xd0, 0xf4, 0xf4, 0xf4, 0xf4, \
  0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, \
  0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4
#define PROXY_PTR ((void*)0x5AFECA110000)
#define PROXY_SIZE NACL_MAP_PAGESIZE
#define PROXY_IDX 2
#define PROXY_PATTERN \
  0x48, 0xb8, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xff, 0xe0
#define NULL_SIZE 0x10000
#define USER_PTR_SIZE sizeof(int32_t)

/* should be kept in sync with struct UserManifest from api/zvm.h*/
struct UserManifest64
{
  uint32_t heap_ptr;
  uint32_t heap_size;
  uint32_t stack_size;
  int32_t channels_count;
  struct ChannelRec channels[0];
};

static intptr_t heap_end = 0;
extern int SyscallSeg(); /* defined in to_trap.S */

/* allocate and set call proxy */
static void MakeTrapProxy()
{
  int i;
  void *p;
  uint8_t pattern[] = { PROXY_PATTERN };

  /* allocate page for proxy */
  p = mmap(PROXY_PTR, PROXY_SIZE, PROT_WRITE, ABSOLUTE_MMAP, -1, 0);
  ZLOGFAIL(PROXY_PTR != p, errno, "cannot allocate proxy");

  /* populate proxy area with halts */
  memset(PROXY_PTR, NACL_HALT_OPCODE, PROXY_SIZE);

  /* patch pattern with trap address and update proxy */
  *(uintptr_t*)(pattern + PROXY_IDX) = (uintptr_t)&SyscallSeg;
  memcpy(PROXY_PTR, pattern, ARRAY_SIZE(pattern));

  /* change proxy protection */
  i = Zmprotect(PROXY_PTR, PROXY_SIZE, PROT_READ | PROT_EXEC | PROT_LOCK);
  ZLOGFAIL(0 != i, i, "cannot set proxy protection");
}

/* initialize trampoline using given thunk address */
static void SetTrampoline()
{
  int i;
  uint8_t pattern[] = { TRAMP_PATTERN };

  /* change protection of area to RW */
  i = Zmprotect((void*)(MEM_START + NACL_TRAMPOLINE_START),
      NACL_TRAMPOLINE_SIZE, PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != i, i, "cannot make trampoline writable");

  /* create trampoline call pattern */
  *(uintptr_t*)(pattern + TRAMP_IDX) = (uintptr_t)PROXY_PTR;

  /* populate trampoline area with it */
  for(i = 0; i < NACL_TRAMPOLINE_SIZE / ARRAY_SIZE(pattern); ++i)
    memcpy((void*)(MEM_START + NACL_TRAMPOLINE_START) + i * ARRAY_SIZE(pattern),
        pattern, ARRAY_SIZE(pattern));

  /* change protection of area to RXL */
  i = Zmprotect((void*)(MEM_START + NACL_TRAMPOLINE_START),
      NACL_TRAMPOLINE_SIZE, PROT_READ | PROT_EXEC | PROT_LOCK);
  ZLOGFAIL(0 != i, i, "cannot make trampoline executable");
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
  p = mmap((void*)UNTRUSTED_START, UNTRUSTED_SIZE, PROT_NONE, ABSOLUTE_MMAP, -1, 0);
  ZLOGFAIL((void*)UNTRUSTED_START != p, errno, "cannot allocate 84gb");

  /* give advice to kernel */
  i = madvise((void*)UNTRUSTED_START, UNTRUSTED_SIZE, MADV_DONTNEED);
  ZLOGIF(i != 0, "cannot madvise 84gb: %s", strerror(errno));

  MakeTrapProxy();
}

void FreeUserSpace()
{
  munmap((void*)UNTRUSTED_START, 2 * GUARDSIZE + FOURGIG);
  FreeTrapProxy();
}

/* initialize stack */
static void SetStack()
{
  int i;

  /* change protection of stack to RWL */
  i = Zmprotect((void*)(MEM_START + FOURGIG - STACK_SIZE),
      STACK_SIZE, PROT_READ | PROT_WRITE | PROT_LOCK);
  ZLOGFAIL(0 != i, i, "cannot set stack protection");
}

/* calculate and allocate user heap. abort if fail */
/* TODO(d'b): should be refactored after "uboot" will be done {{ */
/* WARNING: should be called AFTER elf loaded until SEL will be removed */
static void SetHeapTMP(struct NaClApp *nap)
{
  uintptr_t i;
  int64_t heap;
  void *p;

  /* fail if memory size is invalid */
  ZLOGFAIL(nap->manifest->mem_size <= 0 || nap->manifest->mem_size > FOURGIG,
      ENOMEM, "invalid memory size");

  /* calculate user heap size (must be allocated next to the data_end) */
  p = (void*)ROUNDUP_64K(nap->data_end);
  heap = nap->manifest->mem_size - STACK_SIZE;
  heap = ROUNDUP_64K(heap) - ROUNDUP_64K(nap->data_end);
  ZLOGFAIL(heap <= MIN_HEAP_SIZE, ENOMEM, "user heap size is too small");

  /* make heap RW */
  p = (void*)NaClUserToSys((uintptr_t)p);
  i = Zmprotect(p, heap, PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != i, i, "cannot protection user heap");
  heap_end = NaClSysToUser((uintptr_t)p + heap);
}

/* set user manifest and initialize heap */
static void SetManifest()
{
  struct ChannelsSerial *channels;
  struct UserManifest64 *user;
  int64_t size;
  int i;

  assert(gnap != NULL);
  assert(gnap->manifest != NULL);
  assert(gnap->manifest->channels != NULL);

  /* set user manifest to the user stack end */
  user = (void*)NaClUserToSys(FOURGIG - STACK_SIZE);

  /* serialize channels and copy to user manifest */
  channels = ChannelsSerializer(gnap->manifest, FOURGIG - STACK_SIZE + sizeof *user);
  memcpy(&user->channels, channels->channels, channels->size);

  /* set user manifest fields */
  user->heap_ptr = gnap->break_addr;
  user->channels_count = gnap->manifest->channels->len;
  user->heap_size = heap_end - gnap->break_addr;

  /* calculate and set stack size */
  size = ROUNDUP_64K(channels->size + sizeof *user);
  user->stack_size = STACK_SIZE - size;

  /* make the user manifest read only but not locked for uboot sake */
  /* TODO(d'b): avoid this hack */
  for(i = 0; i < size / NACL_MAP_PAGESIZE; ++i)
    GetUserMap()[i + (FOURGIG - STACK_SIZE) / NACL_MAP_PAGESIZE]
                 &= PROT_READ | PROT_WRITE;
  Zmprotect(user, size, PROT_READ);
}

/* set user manifest and initialize heap */
static void SetHeap()
{
  /* TODO(d'b): wrapper. content of SetHeapTMP() should be moved here */
  SetHeapTMP(gnap);
}

/* set RX protection on user code */
static void SetCode()
{
  int i;

  /* change protection of area to RX */
  /* TODO(d'b): will be removed after finishing "uboot" */
  i = Zmprotect((void*)(MEM_START + NACL_TRAMPOLINE_START + NACL_TRAMPOLINE_SIZE),
      gnap->static_text_end - NACL_TRAMPOLINE_SIZE - NULL_SIZE,
      PROT_READ | PROT_EXEC);
  ZLOGFAIL(0 != i, ENOMEM, "cannot set RX protection on user code");
}

/* set R protection on user R data */
static void SetROData()
{
  int i;

  /* change protection of area to R if read only data is non NULL */
  if(gnap->data_start == 0) return;
  i = Zmprotect((void*)(MEM_START + gnap->rodata_start),
      gnap->data_start - gnap->rodata_start, PROT_READ);
  ZLOGFAIL(0 != i, ENOMEM, "cannot set R protection on user R data");
}
/* }} */

void SetUserSpace()
{
  SetTrampoline();
  SetStack();
  SetHeap();
  SetManifest();
  SetCode(); /* TODO(d'b): remove after "uboot" will be done */
  SetROData(); /* TODO(d'b): remove after "uboot" will be done */
}
