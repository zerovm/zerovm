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
#include "src/main/config.h"
#include "src/main/zlog.h"
#include "src/loader/userspace.h"
#include "src/loader/usermap.h"
#include "src/loader/context.h"
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
#define MANIFEST_PTR_MOCK 0x3d /* cmp eax, ? */
#define MANIFEST_PTR_MOCK_SIZE 1
#define MANIFEST_PTR_MOCK_IDX 12
#define MANIFEST_PTR (FOURGIG - STACK_SIZE)

/* should be kept in sync with struct UserManifest from api/zvm.h */
struct UserManifest64
{
  uint32_t heap_ptr;
  uint32_t heap_size;
  uint32_t stack_size;
  int32_t channels_count;

#ifndef REMOVE_DEPRECATED
  uint32_t channels;
#else
  struct ChannelRec channels[0];
#endif
};

static uintptr_t heap_end = 0;
extern int SyscallSeg(); /* defined in to_trap.S */

uintptr_t UserHeapEnd()
{
  return NaClUserToSys(heap_end);
}

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
  memset(PROXY_PTR, HALT_OPCODE, PROXY_SIZE);

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
  for(i = 1; i < NACL_TRAMPOLINE_SIZE / ARRAY_SIZE(pattern); ++i)
    memcpy((void*)(MEM_START + NACL_TRAMPOLINE_START) + i * ARRAY_SIZE(pattern),
        pattern, ARRAY_SIZE(pattern));

  /* place manifest pointer to trap in trampoline */
  pattern[MANIFEST_PTR_MOCK_IDX] = MANIFEST_PTR_MOCK;
  *(uint32_t*)(pattern + MANIFEST_PTR_MOCK_IDX + MANIFEST_PTR_MOCK_SIZE)
      = MANIFEST_PTR;
  memcpy((void*)(MEM_START + NACL_TRAMPOLINE_START),
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

/* set user manifest and initialize heap */
static void SetManifest(const struct Manifest *manifest)
{
  struct ChannelsSerial *channels;
  struct UserManifest64 *user;
  int64_t size;
  int i;

  assert(manifest != NULL);
  assert(manifest->channels != NULL);

  /* set user manifest to the user stack end */
  user = (void*)NaClUserToSys(MANIFEST_PTR);

  /* serialize channels and copy to user manifest */
#ifndef REMOVE_DEPRECATED
  channels = ChannelsSerializer(manifest, MANIFEST_PTR + sizeof *user);
  user->channels = (uint32_t)(NaClSysToUser((uintptr_t)&user->channels) + sizeof(uint32_t));
  memcpy((void*)NaClUserToSys(MANIFEST_PTR + sizeof *user),
      channels->channels, channels->size);
#else
  channels = ChannelsSerializer(manifest, MANIFEST_PTR + sizeof *user);
  memcpy(&user->channels, channels->channels, channels->size);
#endif

  /* set user manifest fields */
  user->heap_ptr = NACL_TRAMPOLINE_END;
  user->heap_size = heap_end - NACL_TRAMPOLINE_END;
  user->channels_count = manifest->channels->len;

  /* calculate and set stack size */
  size = ROUNDUP_64K(channels->size + sizeof *user);
  user->stack_size = STACK_SIZE - size;

  /* make the user manifest read only but not locked (for uboot) */
  /* TODO(d'b): avoid this hack */
  for(i = 0; i < size / NACL_MAP_PAGESIZE; ++i)
    GetUserMap()[i + MANIFEST_PTR / NACL_MAP_PAGESIZE]
                 &= PROT_READ | PROT_WRITE;

  /*
   * yet another hack to make -F switch useful. when uboot finishes its work
   * it modifies user manifest. if manifest will be locked uboot will fail at
   * line number 314, however that will mean that validation was successful
   */
  i = CommandPtr()->quit_after_load ? PROT_READ | PROT_LOCK : PROT_READ;
  i = Zmprotect(user, size, i);
  ZLOGFAIL(0 != i, i, "cannot set manifest protection");

  g_free(channels);
}

/* calculate and set user heap */
static void SetHeap(const struct Manifest *manifest)
{
  uintptr_t i;
  int64_t heap;
  void *p;

  /* fail if memory size is invalid */
  ZLOGFAIL(manifest->mem_size <= 0 || manifest->mem_size > FOURGIG,
      ENOMEM, "invalid memory size");

  /* calculate user heap size (must be allocated next to the data_end) */
  p = (void*) ROUNDUP_64K(NACL_TRAMPOLINE_END);
  heap = manifest->mem_size - STACK_SIZE;
  heap = ROUNDUP_64K(heap) - ROUNDUP_64K(NACL_TRAMPOLINE_END);
  ZLOGFAIL(heap <= MIN_HEAP_SIZE, ENOMEM, "user heap size is too small");

  /* make heap RW */
  p = (void*) NaClUserToSys(NACL_TRAMPOLINE_END);
  i = Zmprotect(p, heap, PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != i, i, "cannot protection user heap");
  heap_end = NaClSysToUser((uintptr_t) p + heap);
}

#ifndef REMOVE_DEPRECATED
/*
 * place pointer to user manifest to the 0xFEFFFFFC in user address space.
 * only need to support DEPRECATED API version 1. should be removed with
 * DEPRECATED API version 1
 * this patch affects:
 * - user manifest "heap_size"
 * - heap_end
 * - hole (if exist) or heap (if hole does not exist)
 * - user memory map (can unlock locked areas)
 * TODO(d'b): remove it with DEPRECATED API version 1
 */
#define OLD_MFT_PTR 0xFEFFFFFC
static void PatchForOldAPI(struct Manifest *manifest)
{
  int i;
  void *p;

  /* make user manifest writable */
  p = (void*)NaClUserToSys(MANIFEST_PTR);
  i = Zmprotect(p, NACL_MAP_PAGESIZE, PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != i, i, "cannot set user manifest writable");

  /* edit user manifest "heap_size" */
  ((struct UserManifest64*)p)->heap_size -= NACL_MAP_PAGESIZE;
  heap_end -= NACL_MAP_PAGESIZE;

  /* re-protect user manifest */
  i = Zmprotect(p, NACL_MAP_PAGESIZE, PROT_READ);
  ZLOGFAIL(0 != i, i, "cannot protect user manifest");

  /* make page with user manifest pointer writable */
  p = (void*)NaClUserToSys(MANIFEST_PTR - NACL_MAP_PAGESIZE);
  i = Zmprotect(p, NACL_MAP_PAGESIZE, PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != i, i, "cannot set compatibility user manifest pointer");

  /* set user manifest pointer */
  *(uint32_t*)NaClUserToSys(OLD_MFT_PTR) = MANIFEST_PTR;

  /* protect user manifest pointer */
  i = Zmprotect(p, NACL_MAP_PAGESIZE, PROT_READ | PROT_LOCK);
  ZLOGFAIL(0 != i, i, "cannot protect compatibility user manifest pointer");

  /* increase hole if exist */
  if(manifest->mem_size < FOURGIG)
  {
    p = (void*)NaClUserToSys(manifest->mem_size - STACK_SIZE - NACL_MAP_PAGESIZE);
    i = Zmprotect(p, NACL_MAP_PAGESIZE, PROT_NONE | PROT_LOCK);
    ZLOGFAIL(0 != i, i, "cannot add page to hole");
  }
}
#endif

void SetUserSpace(struct Manifest *manifest)
{
  SetTrampoline();
  SetStack();
  SetHeap(manifest);
  SetManifest(manifest);

#ifndef REMOVE_DEPRECATED
  PatchForOldAPI(manifest);
#endif

  LockRestrictedMemory();
}
