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
 * TODO(d'b): the replacement for sel_*. will be activated after
 * SEL will be removed. also this component will be used by snapshot engine
 * TODO(d'b): collect all defines in proper place under proper names
 */

#include <assert.h>
#include <sys/mman.h>
#include "src/loader/sel_ldr.h" // ### to remove
#include "src/loader/userspace.h"
#include "src/loader/usermap.h"
#include "src/channels/channel.h"
#include "src/platform/sel_memory.h"

/*
 * TODO(d'b): reconsider user manifest position. perhaps it worth to place
 * it to the stack bottom
 * pros:
 *   - no need to recalculate and re-protect heap
 *   - no need to recalculate and re-protect hole
 * cons:
 *   - reduction by 64kb..5mb of user stack (more likely by 64kb)
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
#define PROXY_PATTERN \
  0x48, 0xb8, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xff, 0xe0
#define NULL_SIZE 0x10000
#define USER_PTR_SIZE sizeof(int32_t)

/* should be kept in sync with struct ZVMChannel from api/zvm.h */
struct ChannelSerialized
{
  int64_t limits[LimitsNumber];
  int64_t size;
  uint32_t type;
  uint32_t name;
};

/* should be kept in sync with struct UserManifest from api/zvm.h*/
struct UserManifestSerialized
{
  uint32_t heap_ptr;
  uint32_t heap_size;
  uint32_t stack_size;
  int32_t channels_count;
  uint32_t channels;
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
  i = NaCl_mprotect(PROXY_PTR, PROXY_SIZE, PROT_READ | PROT_EXEC);
  ZLOGFAIL(0 != i, ENOMEM, "cannot set stack protection");
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
    memcpy((void*)(MEM_START + NACL_TRAMPOLINE_START) + i * ARRAY_SIZE(pattern),
        pattern, ARRAY_SIZE(pattern));

  /* change protection of area to RXL */
  i = NaCl_mprotect((void*)(MEM_START + NACL_TRAMPOLINE_START),
      NACL_TRAMPOLINE_SIZE, PROT_READ | PROT_EXEC | PROT_LOCK);
  ZLOGFAIL(0 != i, ENOMEM, "cannot make trampoline executable");
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
  p = mmap(USER_PTR, SIZE_84GB, PROT_NONE, ABSOLUTE_MMAP, -1, 0);
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

/* initialize stack */
static void SetStack()
{
  int i;

  /* change protection of stack to RWL */
  i = NaCl_mprotect((void*)(MEM_START + FOURGIG - STACK_SIZE),
      STACK_SIZE, PROT_READ | PROT_WRITE | PROT_LOCK);
  ZLOGFAIL(0 != i, ENOMEM, "cannot set stack protection");
}

/* calculate and allocate user heap. abort if fail */
/* TODO(d'b): should be refactored after "simple boot" will be done {{ */
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
  i = NaCl_mprotect(p, heap, PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != i, -i, "cannot set protection on user heap");
  heap_end = NaClSysToUser((uintptr_t)p + heap);
}

/* set pointer to user manifest */
static void SetUserManifestPtr(struct NaClApp *nap, void *mft)
{
  uintptr_t *p;

  p = (void*)NaClUserToSys(FOURGIG - STACK_SIZE - USER_PTR_SIZE);
  *p = NaClSysToUser((uintptr_t)mft);
}

/* align area to page(s) both bounds an protect */
static void AlignAndProtect(uintptr_t area, int64_t size, int prot)
{
  uintptr_t page_ptr;
  uint64_t aligned_size;
  int code;

  page_ptr = ROUNDDOWN_64K(area);
  aligned_size = ROUNDUP_64K(size + (area - page_ptr));

  code = NaCl_mprotect((void*)page_ptr, aligned_size, prot);
  ZLOGFAIL(0 != code, code, "cannot protect 0x%x of %d bytes with %d",
      page_ptr, aligned_size, prot);
}

/*
 * copy given name before the given area making it writable 1st
 * return a new pointer to name preceding area
 */
static void *CopyDown(void *area, char *name)
{
  int size = strlen(name) + 1;

  /* when the page crossed, append the new one */
  if((uintptr_t) area - ROUNDDOWN_64K((uintptr_t)area) < size)
    AlignAndProtect((uintptr_t) area - size, size, PROT_READ | PROT_WRITE);

  /* copy name down */
  return memcpy((char*)area - size, name, size);
}

/* protect user manifest with RL */
static void ProtectUserManifest(struct NaClApp *nap, void *mft)
{
  uint64_t size;

  size = FOURGIG - STACK_SIZE - NaClSysToUser((uintptr_t)mft);
  AlignAndProtect((uintptr_t) mft, size, PROT_READ | PROT_LOCK);
}

static void SetSystemData(struct NaClApp *nap)
{
  struct Manifest *manifest;
  struct ChannelSerialized *channels;
  struct UserManifestSerialized *user_manifest;
  void *ptr; /* pointer to the user manifest area */
  int64_t size;
  int i;

  assert(nap != NULL);
  assert(nap->manifest != NULL);
  assert(nap->manifest->channels != NULL);

  manifest = nap->manifest;

  /*
   * 1. calculate channels array size (w/o aliases)
   * 2. calculate user manifest size (w/o aliases)
   * 3. calculate pointer to user manifest
   * 4. calculate pointer to channels array
   */
  size = manifest->channels->len * sizeof(struct ChannelSerialized);
  size += sizeof(struct UserManifestSerialized) + USER_PTR_SIZE;
  ptr = (void*)(FOURGIG - STACK_SIZE - size);
  user_manifest = (void*)NaClUserToSys((uintptr_t)ptr);
  channels = (void*)((uintptr_t)&user_manifest->channels + USER_PTR_SIZE);

  /* make the 1st page of user manifest writable */
  CopyDown((void*)NaClUserToSys(FOURGIG - STACK_SIZE), "");
  ptr = user_manifest;

  /* initialize pointer to user manifest */
  SetUserManifestPtr(nap, user_manifest);

  /* serialize channels */
  for(i = 0; i < manifest->channels->len; ++i)
  {
    /* limits */
    int j;
    for(j = 0; j < LimitsNumber; ++j)
      channels[i].limits[j] = CH_CH(manifest, i)->limits[j];

    /* type and size */
    channels[i].type = (int32_t)CH_CH(manifest, i)->type;
    channels[i].size = channels[i].type == SGetSPut
        ? 0 : CH_CH(manifest, i)->size;

    /* alias */
    ptr = CopyDown(ptr, CH_CH(manifest, i)->alias);
    channels[i].name = NaClSysToUser((uintptr_t)ptr);
  }

  /* update heap_size in the user manifest */
  size = ROUNDDOWN_64K(NaClSysToUser((uintptr_t)ptr));
  size = MIN(heap_end, size);
  user_manifest->heap_size = size - nap->break_addr;

  /* serialize the rest of the user manifest records */
  user_manifest->heap_ptr = nap->break_addr;
  user_manifest->stack_size = STACK_SIZE;
  user_manifest->channels_count = manifest->channels->len;
  user_manifest->channels = NaClSysToUser((uintptr_t)channels);

  /* make the user manifest read only */
  ProtectUserManifest(nap, ptr);
}

/* set user manifest and initialize heap */
static void SetManifest()
{
  /* TODO(d'b): wrapper. content of SetSystemData() should be moved here */
  SetSystemData(gnap);
}

/* set user manifest and initialize heap */
static void SetHeap()
{
  /* TODO(d'b): wrapper. content of SetHeapTMP() should be moved here */
  SetHeapTMP(gnap);
}

/* ### set RX protection on user code */
static void SetCode()
{
  int i;

  /* change protection of area to RX */
  i = NaCl_mprotect((void*)(MEM_START + NACL_TRAMPOLINE_START + NACL_TRAMPOLINE_SIZE),
      gnap->static_text_end - NACL_TRAMPOLINE_SIZE - NULL_SIZE,
      PROT_READ | PROT_EXEC);
  ZLOGFAIL(0 != i, ENOMEM, "cannot set RX protection on user code");
}

/* ### set R protection on user R data */
static void SetROData()
{
  int i;

  /* change protection of area to R if read only data is non NULL */
  if(gnap->data_start == 0) return;
  i = NaCl_mprotect((void*)(MEM_START + gnap->rodata_start),
      gnap->data_start - gnap->rodata_start, PROT_READ);
  ZLOGFAIL(0 != i, ENOMEM, "cannot set R protection on user R data");
}
/* }} */

void SetUserSpace()
{
  SetTrampoline();
  SetStack();
  SetHeap();
  ChannelsCtor(gnap->manifest); // ### find proper place and method to call it
  SetManifest();
  SetCode(); /* TODO(d'b): remove after "simple boot" will be done */
  SetROData(); /* TODO(d'b): remove after "simple boot" will be done */
}
