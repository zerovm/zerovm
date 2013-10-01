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

/* hard limit for all zerovm i/o */
static int64_t storage_limit = ZEROVM_IO_LIMIT;
static GTimer *timer = NULL;
static FILE *ztrace_log = NULL;

void ZTraceCtor(const char *name)
{
  ZLOGFAIL(!g_path_is_absolute(name), EFAULT,
      "ztrace log should have absolute path");
  ztrace_log = fopen(name, "a");
  ZLOGFAIL(ztrace_log == NULL, errno, "cannot open %s", name);
  fprintf(ztrace_log, "\n[%d] %048o\n", getpid(), 0);
  timer = g_timer_new();
}

void ZTraceDtor()
{
  if(timer != NULL) g_timer_destroy(timer);
  if(ztrace_log != NULL) fclose(ztrace_log);
}

void ZTrace(const char *msg)
{
  static double chrono = 0;
  double timing;

  if(timer == NULL || ztrace_log == NULL) return;

  timing = g_timer_elapsed(timer, NULL);
  chrono += timing;
  fprintf(ztrace_log, "%.6f [%.6f]: %s\n", chrono, timing, msg);
  g_timer_start(timer);
}

/* limit zerovm i/o */
static void LimitOwnIO()
{
  struct rlimit rl;

  assert(storage_limit > 0);

  ZLOGFAIL(getrlimit(RLIMIT_FSIZE, &rl) != 0, errno, "cannot get RLIMIT_FSIZE");
  rl.rlim_cur = storage_limit;
  ZLOGFAIL(setrlimit(RLIMIT_FSIZE, &rl) != 0, errno, "cannot set RLIMIT_FSIZE");
}

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

int SetStorageLimit(int64_t a)
{
  if(a < 1) return -1;

  storage_limit = a * ZEROVM_IO_LIMIT_UNIT;
  return 0;
}

void LastDefenseLine(struct NaClApp *nap)
{
  LimitOwnIO();
  SetTimeout(nap->manifest);
  LowerOwnPriority();
  GiveUpPrivileges();
}

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
  heap = nap->manifest->mem_size - nap->stack_size;
  heap = ROUNDUP_64K(heap) - ROUNDUP_64K(nap->data_end);
  ZLOGFAIL(heap <= LEAST_USER_HEAP_SIZE, ENOMEM, "user heap size is too small");

  /* since 4gb of user space is already allocated just set protection to the heap */
  p = (void*)NaClUserToSys(nap, (uintptr_t)p);
  i = NaCl_mprotect(p, heap, PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != i, -i, "cannot set protection on user heap");
  nap->heap_end = NaClSysToUser(nap, (uintptr_t)p + heap);

  nap->mem_map[HeapIdx].size += heap;
  nap->mem_map[HeapIdx].end += heap;
}

/* TODO(d'b): move it to sel_addrspace */
/* should be kept in sync with api/zvm.h*/
struct ChannelSerialized
{
  int64_t limits[LimitsNumber];
  int64_t size;
  uint32_t type;
  uint32_t name;
};

/* should be kept in sync with api/zvm.h*/
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

/* set pointer to user manifest */
static void SetUserManifestPtr(struct NaClApp *nap, void *mft)
{
  uintptr_t *p;

  p = (void*)NaClUserToSys(nap, FOURGIG - nap->stack_size - USER_PTR_SIZE);
  *p = NaClSysToUser(nap, (uintptr_t)mft);
}

/* align area to page(s) both bounds an protect. return pointer to (new) area */
static uintptr_t AlignAndProtect(uintptr_t area, int64_t size, int prot)
{
  uintptr_t page_ptr;
  uint64_t aligned_size;
  int code;

  page_ptr = ROUNDDOWN_64K(area);
  aligned_size = ROUNDUP_64K(size + (area - page_ptr));

  code = NaCl_mprotect((void*)page_ptr, aligned_size, prot);
  ZLOGFAIL(0 != code, code, "cannot protect 0x%x of %d bytes with %d",
      page_ptr, aligned_size, prot);
  return page_ptr;
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

/* protect user manifest and update mem_map */
static void ProtectUserManifest(struct NaClApp *nap, void *mft)
{
  uintptr_t page_ptr;
  uint64_t size;

  size = FOURGIG - nap->stack_size - NaClSysToUser(nap, (uintptr_t)mft);
  page_ptr = AlignAndProtect((uintptr_t) mft, size, PROT_READ);

  /* update mem_map */
  SET_MEM_MAP_IDX(nap->mem_map[SysDataIdx], "UserManifest",
      page_ptr, ROUNDUP_64K(size + ((uintptr_t)mft - page_ptr)), PROT_READ);

  /* its time to add hole to memory map */
  page_ptr = nap->mem_map[HeapIdx].end;
  size = nap->mem_map[SysDataIdx].start - nap->mem_map[HeapIdx].end;
  SET_MEM_MAP_IDX(nap->mem_map[HoleIdx], "Hole", page_ptr, size, PROT_NONE);

  /*
   * patch: change the heap size to correct value. the user manifest
   * contains the different heap start (it does not include r/w data)
   * TODO(d'b): fix it by adding a new memory region
   */
  nap->mem_map[HeapIdx].size =
      nap->mem_map[HeapIdx].end - nap->mem_map[HeapIdx].start;
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
  ptr = (void*)(FOURGIG - nap->stack_size - size);
  user_manifest = (void*)NaClUserToSys(nap, (uintptr_t)ptr);
  channels = (void*)((uintptr_t)&user_manifest->channels + USER_PTR_SIZE);

  /* make the 1st page of user manifest writable */
  CopyDown((void*)NaClUserToSys(nap, FOURGIG - nap->stack_size), "");
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
    channels[i].name = NaClSysToUser(nap, (uintptr_t)ptr);
  }

  /* update heap_size in the user manifest */
  size = ROUNDDOWN_64K(NaClSysToUser(nap, (uintptr_t)ptr));
  size = MIN(nap->heap_end, size);
  user_manifest->heap_size = size - nap->break_addr;

  /* note that rw data merged with heap! */

  /* update memory map */
  nap->mem_map[HeapIdx].end = NaClUserToSys(nap, size);
  nap->mem_map[HeapIdx].size = user_manifest->heap_size;

  /* serialize the rest of the user manifest records */
  user_manifest->heap_ptr = nap->break_addr;
  user_manifest->stack_size = nap->stack_size;
  user_manifest->channels_count = manifest->channels->len;
  user_manifest->channels = NaClSysToUser(nap, (uintptr_t)channels);

  /* make the user manifest read only */
  ProtectUserManifest(nap, ptr);
}
/* }} */
