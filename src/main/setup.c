/*
 * construct system manifest and host manifest
 * system/channels resources counters are also here
 *
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
#include <sys/resource.h>
#include <sys/mman.h>
#include "src/loader/sel_ldr.h"
#include "src/platform/sel_memory.h"
#include "src/main/setup.h"
#include "src/channels/channel.h"

/*
 * ### {{
 * new user memory manager. replaces struct MemBlock (with all its
 * macros/logic). need to be done for snapshot.
 * NOTE: "addr" should be specified in system address space
 * TODO(d'b): should be moved to sel_memory.*
 * TODO(d'b): map updating and checking is slow, speed it up
 */

/* contains PROT_* bits of user pages */
static uint8_t user_map[FOURGIG / NACL_MAP_PAGESIZE];

uint8_t *GetUserMap()
{
  uint8_t *buffer = g_malloc(ARRAY_SIZE(user_map));
  ZLOGFAIL(buffer == NULL, ENOMEM, "cannot allocate user map copy");
  memcpy(buffer, user_map, ARRAY_SIZE(user_map));

  return buffer;
}

void LockRestrictedMemory()
{
  int i;

  /* NULL and hole (everything protected with PROT_NONE is restricted) */
  for(i = 0; i < FOURGIG / NACL_MAP_PAGESIZE; ++i)
    if(user_map[i] == PROT_NONE)
      user_map[i] = PROT_LOCK;

  /* stack */
  memset(user_map + (FOURGIG - STACK_SIZE) / NACL_MAP_PAGESIZE,
      PROT_READ |PROT_WRITE |PROT_LOCK, STACK_SIZE / NACL_MAP_PAGESIZE);

  /* trampoline */
  user_map[1] |= PROT_LOCK;

  /* TODO(d'b): manifest can be larger than 1 page. fix it! */
  user_map[(FOURGIG - STACK_SIZE - 1) / NACL_MAP_PAGESIZE] |= PROT_LOCK;
}

/* return index of "addr" in user map */
INLINE static int UserMapIndex(uintptr_t addr)
{
  return (addr - (uintptr_t)R15_CONST - GUARDSIZE) / NACL_MAP_PAGESIZE;
}

int CheckUserMap(uintptr_t addr, uint32_t size, int prot)
{
  int i;

  /* check arguments sanity */
  if(addr - MEM_START < 0) // ### does it work?
    return -1;
  if(addr - MEM_START + size > FOURGIG)
    return -1;
  if(prot < PROT_NONE || prot > (PROT_READ|PROT_WRITE|PROT_EXEC))
    return -1;

  /* scan given region for availability */
  if(prot == 0)
  {
    for(i = 0; i < ROUNDUP_64K(size) / NACL_MAP_PAGESIZE; ++i)
      if(user_map[UserMapIndex(addr) + i] & PROT_LOCK)
        return -1;
  }
  /* scan given region for prot */
  else
    for(i = 0; i < ROUNDUP_64K(size) / NACL_MAP_PAGESIZE; ++i)
      if((user_map[UserMapIndex(addr) + i] & prot) == 0)
        return -1;

  return 0;
}

int UpdateUserMap(uintptr_t addr, uint32_t size, int prot)
{
  int i;

  /* check if all pages available */
  if(CheckUserMap(addr, size, PROT_NONE) != 0) return -1;

  /* set new protection for all pages */
  for(i = 0; i < ROUNDUP_64K(size) / NACL_MAP_PAGESIZE; ++i)
    user_map[UserMapIndex(addr) + i] = (uint8_t)prot;
  return 0;
}

/* }} */

/* set timeout. by design timeout must be specified in manifest */
static void SetTimeout(struct Manifest *manifest)
{
  assert(manifest != NULL);

  ZLOGFAIL(manifest->timeout < 1, EFAULT, "invalid timeout %d", manifest->timeout);
  alarm(manifest->timeout);
}

/* lower zerovm priority */
static void LowerOwnPriority()
{
  ZLOGFAIL(setpriority(PRIO_PROCESS, 0, ZEROVM_PRIORITY) != 0,
      errno, "cannot set zerovm priority");
}

/*
 * give up privileges
 * TODO(d'b): instead of fail set group/user to nogroup/nobody
 */
static void GiveUpPrivileges()
{
  ZLOGFAIL(getuid() == 0, EPERM, "zerovm is not permitted to run as root");
}

void LastDefenseLine(struct Manifest *manifest)
{
  SetTimeout(manifest);
  LowerOwnPriority();
  GiveUpPrivileges();
}

/* TODO(d'b): move it to sel_addrspace */
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

#define USER_PTR_SIZE sizeof(int32_t)
#define CHANNEL_STRUCT_SIZE sizeof(struct ChannelSerialized)
#define USER_MANIFEST_STRUCT_SIZE sizeof(struct UserManifestSerialized)

static intptr_t heap_end = 0;

void PreallocateUserMemory(struct NaClApp *nap)
{
  uintptr_t i;
  int64_t heap;
  void *p;

  assert(nap != NULL);
  assert(nap->manifest != NULL);

  /* quit function if "Memory" is not specified or invalid */
  ZLOGFAIL(nap->manifest->mem_size <= 0 || nap->manifest->mem_size > FOURGIG,
      ENOMEM, "invalid memory size");

  /* calculate user heap size (must be allocated next to the data_end) */
  p = (void*)ROUNDUP_64K(nap->data_end);
  heap = nap->manifest->mem_size - STACK_SIZE;
  heap = ROUNDUP_64K(heap) - ROUNDUP_64K(nap->data_end);
  ZLOGFAIL(heap <= LEAST_USER_HEAP_SIZE, ENOMEM, "user heap size is too small");

  /* since 4gb of user space is already allocated just set protection to the heap */
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

/* protect user manifest */
static void ProtectUserManifest(struct NaClApp *nap, void *mft)
{
  uint64_t size;

  size = FOURGIG - STACK_SIZE - NaClSysToUser((uintptr_t)mft);
  AlignAndProtect((uintptr_t) mft, size, PROT_READ);
}

void SetSystemData(struct NaClApp *nap)
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
  size = manifest->channels->len * CHANNEL_STRUCT_SIZE;
  size += USER_MANIFEST_STRUCT_SIZE + USER_PTR_SIZE;
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
/* }} */
