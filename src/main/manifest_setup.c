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
/* todo(d'b): should merge into sel_addrspace, sel_ldr, nacl_exit e.t.c. */
#include <assert.h>
#include <time.h>
#include <sys/resource.h> /* timeout, process priority */
#include <sys/mman.h>
#include <glib.h>
#include "src/loader/sel_ldr.h"
#include "src/main/etag.h"
#include "src/platform/sel_memory.h"
#include "src/main/manifest_parser.h"
#include "src/main/manifest_setup.h"
#include "src/channels/mount_channel.h"
#include "src/main/accounting.h"
#include "src/main/tools.h"

/* get integer value by key from the manifest. 0 if not found */
#define GET_INT_BY_KEY(var, str) \
  do {\
    char *p = GetValueByKey(str);\
    var = p == NULL ? 0 : g_ascii_strtoll(p, NULL, 10);\
  } while(0)

/* hard limit for all zerovm i/o */
static int64_t storage_limit = ZEROVM_IO_LIMIT;

/* lower zerovm priority */
static void LowerOwnPriority()
{
  ZLOGIF(setpriority(PRIO_PROCESS, 0, ZEROVM_PRIORITY) != 0,
      "cannot lower zerovm priority");
}

int SetStorageLimit(int64_t a)
{
  if(a < 1) return -1;

  storage_limit = a * ZEROVM_IO_LIMIT_UNIT;
  return 0;
}

/* limit zerovm i/o */
static void LimitOwnIO()
{
  struct rlimit rl;

  assert(storage_limit > 0);

  ZLOGIF(getrlimit(RLIMIT_FSIZE, &rl) != 0, "cannot get i/o limits");
  rl.rlim_cur = storage_limit;
  ZLOGIF(setrlimit(RLIMIT_FSIZE, &rl) != 0, "cannot set i/o limits");
}

/* set timeout. by design timeout must be specified in manifest */
static void SetTimeout(struct SystemManifest *policy)
{
  struct rlimit rl;

  assert(policy != NULL);

  GET_INT_BY_KEY(policy->timeout, MFT_TIMEOUT);
  ZLOGFAIL(policy->timeout < 1, EFAULT, "invalid or absent timeout");
  rl.rlim_cur = policy->timeout;
  rl.rlim_max = -1;
  setrlimit(RLIMIT_CPU, &rl);
  ZLOGFAIL(setrlimit(RLIMIT_CPU, &rl) != 0, errno, "cannot set timeout");
}

void LastDefenseLine(struct NaClApp *nap)
{
  LowerOwnPriority();
  LimitOwnIO();
  SetTimeout(nap->system_manifest);
}

/* preallocate memory area of given size. abort if fail */
static void PreallocateUserMemory(struct NaClApp *nap)
{
  uintptr_t i;
  int64_t heap;
  void *p;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /* quit function if max_mem is not specified or invalid */
  ZLOGFAIL(nap->heap_end == 0, ENOMEM, "user memory size invalid or not specified");
  ZLOGFAIL(nap->heap_end > FOURGIG, ENOMEM, "user memory size is too large");

  /* calculate user heap size (must be allocated next to the data_end) */
  p = (void*)NaClRoundAllocPage(nap->data_end);
  heap = nap->heap_end - nap->stack_size;
  heap = NaClRoundAllocPage(heap) - NaClRoundAllocPage(nap->data_end);
  ZLOGFAIL(heap <= LEAST_USER_HEAP_SIZE, ENOMEM, "user heap size is too small");

  /* since 4gb of user space is already allocated just set protection to the heap */
  p = (void*)NaClUserToSys(nap, (uintptr_t)p);
  i = NaCl_mprotect(p, heap, PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != i, -i, "cannot set protection on user heap");
  nap->heap_end = NaClSysToUser(nap, (uintptr_t)p + heap);

  nap->mem_map[HeapIdx].size += heap;
  nap->mem_map[HeapIdx].end += heap;
}

/* helper. sets custom user attributes for user */
static void SetCustomAttributes(struct SystemManifest *policy)
{
  int i;
  int count;
  char *environment;
  char *buf[BIG_ENOUGH_SPACE], **tokens = buf;

  assert(policy != NULL);
  assert(policy->envp == NULL);

  /* get environment */
  environment = GetValueByKey(MFT_ENVIRONMENT);
  if(environment == NULL) return;

  /* parse and check count of attributes */
  count = ParseValue(environment, ", \t", tokens, BIG_ENOUGH_SPACE);
  ZLOGFAIL(count % 2 != 0, EFAULT, "odd number of user environment variables");
  ZLOGFAIL(count == 0, EFAULT, "invalid user environment");
  ZLOGFAIL(count == BIG_ENOUGH_SPACE, EFAULT, "user environment exceeded the limit");

  /* allocate space to hold string pointers */
  count >>= 1;
  policy->envp = g_malloc0((count + 1) * sizeof(*policy->envp));

  /* construct array of environment variables */
  for(i = 0; i < count; ++i)
  {
    char *key = *tokens++;
    char *value = *tokens++;
    int length = strlen(key) + strlen(value) + 1 + 1; /* + '=' + '\0' */

    policy->envp[i] = g_malloc0((length + 1) * sizeof(*policy->envp[0]));
    sprintf(policy->envp[i], "%s=%s", key, value);
  }
}

/* sets node_id in nap object and user argv[0] */
static void SetNodeName(struct NaClApp *nap)
{
  int i;
  char *buf[BIG_ENOUGH_SPACE], **tokens = buf;
  char *pgm_name = GetValueByKey(MFT_NODE);

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->system_manifest->cmd_line != NULL);
  assert(nap->system_manifest->cmd_line_size > 0);

  /* set the node name (0st parameter) and node id (n/a for the user) */
  if(pgm_name == NULL)
  {
    nap->system_manifest->cmd_line[0] = NEXE_PGM_NAME;
    nap->system_manifest->node_id = 0;
  }
  else
  {
    i = ParseValue(pgm_name, ",", tokens, BIG_ENOUGH_SPACE);
    ZLOGFAIL(i != 2, EFAULT, "invalid NodeName specified");
    ZLOGFAIL(tokens[0] == NULL, EFAULT, "invalid node name");
    ZLOGFAIL(tokens[1] == NULL, EFAULT, "invalid node id");
    nap->system_manifest->cmd_line[0] = tokens[0];
    nap->system_manifest->node_id = ATOI(tokens[1]);
    ZLOGFAIL(nap->system_manifest->node_id == 0, EFAULT, "node id must be > 0");
  }

  /* put node name and id to the log */
  ZLOGS(LOG_DEBUG, "node name = %s, node id = %d",
      nap->system_manifest->cmd_line[0], nap->system_manifest->node_id);
}

/* helper. sets command line parameters for the user */
static void SetCommandLine(struct SystemManifest *policy)
{
  int i;
  char *parameters;
  char *buf[BIG_ENOUGH_SPACE], **tokens = buf;

  assert(policy != NULL);
  assert(policy->cmd_line == NULL);
  assert(policy->cmd_line_size == 0);

  /* get parameters */
  parameters = GetValueByKey(MFT_COMMANDLINE);

  /* if there is command line parse and check count of parameters */
  if(parameters != NULL)
  {
    policy->cmd_line_size = ParseValue(parameters, " \t", tokens, BIG_ENOUGH_SPACE);
    ZLOGFAIL(policy->cmd_line_size == 0, EFAULT, "invalid user parameters");
    ZLOGFAIL(policy->cmd_line_size == BIG_ENOUGH_SPACE, EFAULT, "too long user command line");
  }

  /*
   * allocate space to hold string pointers. 0st element reserved for argv[0].
   * also, the last element should be NULL so 1 extra element must be reserved
   */
  ++policy->cmd_line_size;
  policy->cmd_line = g_malloc0((policy->cmd_line_size + 1) * sizeof *policy->cmd_line);

  /* populate command line arguments array with pointers */
  for(i = 0; i < policy->cmd_line_size - 1; ++i)
    policy->cmd_line[i + 1] = tokens[i];
}

/* todo(d'b): move it to sel_addrspace */
/* should be kept in sync with api/zvm.h*/
struct ChannelSerialized
{
  int64_t limits[IOLimitsCount];
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
 * copy given name before the given area making it writeable 1st
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
   * todo(d'b): fix it by adding a new memory region
   */
  nap->mem_map[HeapIdx].size =
      nap->mem_map[HeapIdx].end - nap->mem_map[HeapIdx].start;
}

/* serialize system data to user space */
static void SetSystemData(struct NaClApp *nap)
{
  struct SystemManifest *manifest;
  struct ChannelSerialized *channels;
  struct UserManifestSerialized *user_manifest;
  void *ptr; /* pointer to the user manifest area */
  int64_t size;
  int i;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->system_manifest->channels != NULL);

  manifest = nap->system_manifest;

  /*
   * 1. calculate channels array size (w/o aliases)
   * 2. calculate user manifest size (w/o aliases)
   * 3. calculate pointer to user manifest
   * 4. calculate pointer to channels array
   */
  size = manifest->channels_count * CHANNEL_STRUCT_SIZE;
  size += USER_MANIFEST_STRUCT_SIZE + USER_PTR_SIZE;
  ptr = (void*)(FOURGIG - nap->stack_size - size);
  user_manifest = (void*)NaClUserToSys(nap, (uintptr_t)ptr);
  channels = (void*)((uintptr_t)&user_manifest->channels + USER_PTR_SIZE);

  /* make the 1st page of user manifest writeable */
  CopyDown((void*)NaClUserToSys(nap, FOURGIG - nap->stack_size), "");
  ptr = user_manifest;

  /* initialize pointer to user manifest */
  SetUserManifestPtr(nap, user_manifest);

  /* serialize channels */
  for(i = 0; i < manifest->channels_count; ++i)
  {
    /* limits */
    int j;
    for(j = 0; j < IOLimitsCount; ++j)
      channels[i].limits[j] = manifest->channels[i].limits[j];

    /* type and size */
    channels[i].type = (int32_t)manifest->channels[i].type;
    channels[i].size = channels[i].type == SGetSPut
        ? 0 : manifest->channels[i].size;

    /* alias */
    ptr = CopyDown(ptr, manifest->channels[i].alias);
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
  user_manifest->channels_count = manifest->channels_count;
  user_manifest->channels = NaClSysToUser(nap, (uintptr_t)channels);

  /* make the user manifest read only */
  ProtectUserManifest(nap, ptr);
}
/* }} */

void SystemManifestCtor(struct NaClApp *nap)
{
  struct SystemManifest *policy;

  /* check for design errors */
  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  policy = nap->system_manifest;

  /* get zerovm settings from manifest */
  policy->version = GetValueByKey(MFT_VERSION);
  policy->nexe_etag = GetValueByKey(MFT_ETAG);

  /* check mandatory manifest keys */
  ZLOGFAIL(nap->system_manifest->version == NULL, EFAULT,
      "the manifest version is not provided");
  ZLOGFAIL(g_strcmp0(nap->system_manifest->version, MANIFEST_VERSION),
      EFAULT, "manifest version not supported");

  /* user data (environment, command line) */
  policy->envp = NULL;
  SetCustomAttributes(policy);

  /* prepare command line arguments for nexe */
  policy->cmd_line = NULL;
  policy->cmd_line_size = 0;
  SetCommandLine(policy);

  /* get node name and id */
  SetNodeName(nap);

  /* construct and initialize all channels */
  ChannelsCtor(nap);

  /*
   * allocate "whole memory chunk" if specified. should be the last allocation
   * in raw because after chunk allocated there will be no free user memory
   * note: will set "heap_ptr"
   */
  GET_INT_BY_KEY(nap->heap_end, MFT_MEMORY);
  PreallocateUserMemory(nap);

  /* set user manifest in user space (new ZVM API) */
  SetSystemData(nap);
}

int SystemManifestDtor(struct NaClApp *nap)
{
  assert(nap != NULL);

  if(nap->system_manifest != NULL)
    ChannelsDtor(nap);

  return 0;
}

/* updates user_tag (should be constructed) with memory chunk data */
static void EtagMemoryChunk(struct NaClApp *nap)
{
  int i;

  assert(nap != NULL);
  assert(MEMORY_ETAG_ENABLED != 0);

  for(i = 0; i < MemMapSize; ++i)
  {
    uintptr_t addr = nap->mem_map[i].start;
    int64_t size = nap->mem_map[i].size;

    /* update user_etag skipping inaccessible pages */
    if(nap->mem_map[i].prot & PROT_READ)
      TagUpdate(nap->user_tag, (const char*) addr, size);
  }
}

/* updates user_tag (should be constructed) with all channels digests */
static void ChannelsDigest(struct NaClApp *nap)
{
  int i;

  assert(nap != NULL);

  for(i = 0; i < nap->system_manifest->channels_count; ++i)
    TagUpdate(nap->user_tag,
        (const char*) nap->system_manifest->channels[i].digest, TAG_DIGEST_SIZE);
}

int ProxyReport(struct NaClApp *nap)
{
  char report[BIG_ENOUGH_STRING];
  char etag[TAG_DIGEST_SIZE] = TAG_ENGINE_DISABLED;
  int length;
  int i;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /* tag user memory / channels if session successful */
  if(TagEngineEnabled() && STREQ(OK_STATE, GetExitState()))
  {
    if(CHANNELS_ETAG_ENABLED) ChannelsDigest(nap);
    if(MEMORY_ETAG_ENABLED) EtagMemoryChunk(nap);
    TagDigest(nap->user_tag, etag);
    TagDtor(nap->user_tag);
  }

  /* for debugging purposes it is useful to see more advanced information */
#ifdef DEBUG
  length = g_snprintf(report, BIG_ENOUGH_STRING,
      "validator state = %d\nuser return code = %d\netag = %s\naccounting = %s\n"
      "exit state = %s\n", nap->validation_state,
      nap->system_manifest->user_ret_code, etag, GetAccountingInfo(), GetExitState());
#else
  /* .. but for production zvm will switch to more brief output */
  length = g_snprintf(report, BIG_ENOUGH_STRING, "%d\n%d\n%s\n%s\n%s\n",
      nap->validation_state, nap->system_manifest->user_ret_code,
      etag, GetAccountingInfo(), GetExitState());
#endif

  /* give the report to proxy */
  i = write(STDOUT_FILENO, report, length);

  /* log the report */
  length = g_snprintf(report, BIG_ENOUGH_STRING,
      "validator state = %d, user return code = %d, etag = %s, accounting = %s, "
      "exit state = %s", nap->validation_state,
      nap->system_manifest->user_ret_code, etag, GetAccountingInfo(), GetExitState());
  ZLOGS(LOG_DEBUG, "%s", report);

  return i == length ? 0 : -1;
}
