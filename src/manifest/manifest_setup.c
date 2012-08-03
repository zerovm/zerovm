/*
 * construct system manifest and host manifest
 * system/channels resources counters are also here
 *
 *  Created on: Nov 30, 2011
 *      Author: d'b
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h> /* timeout */
#include <sys/time.h> /* timeout */
#include <unistd.h> /* timeout */

#include "src/utils/tools.h"
#include "src/service_runtime/include/bits/mman.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/nacl_config.h"
#include "src/service_runtime/sel_mem.h"
#include "src/service_runtime/nacl_memory_object.h"
#include "src/platform/nacl_log.h"
#include "src/service_runtime/nacl_syscall_handlers.h"

#include <src/manifest/manifest_keywords.h>
#include <src/manifest/manifest_parser.h>
#include <src/manifest/manifest_setup.h>
#include <src/manifest/mount_channel.h>

#if 0 /* disabled until "snapshot" engine will be done */
/* return md5 hash of mapped _output_ file (or NULL) */
static char* MakeEtag(struct NaClApp *nap)
{
  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  return "disabled";
}
#endif

/* preallocate memory area of given size. abort if fail */
static void PreallocateUserMemory(struct NaClApp *nap)
{
  uintptr_t i;
  uint32_t stump;
  uint32_t dead_space;
  struct NaClVmmapEntry *user_space;

  /* quit function if max_mem is not specified in manifest */
  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  nap->system_manifest->heap_ptr = (uint32_t)(intptr_t)NULL;
  if(nap->system_manifest->max_mem == 0) return;

  /* user memory chunk must be allocated next to the data end */
  stump = nap->system_manifest->max_mem - nap->stack_size - nap->data_end;
  i = nap->data_end;
  /* todo(d'b): make it macro or inline function */
  i = (i + NACL_MAP_PAGESIZE - 1) & ~(NACL_MAP_PAGESIZE - 1);
  nap->system_manifest->heap_ptr =
      NaClCommonSysMmapIntern(nap, (void*)i, stump, 3, 0x22, -1, 0);
  COND_ABORT(nap->system_manifest->heap_ptr != i,
      "cannot allocate specified user memory chunk");

  /*
   * free "whole chunk" block without real memory deallocation
   * the map entry we need is the last in raw
   */
  user_space = nap->mem_map.vmentry[nap->mem_map.nvalid - 1];
  assert(nap->system_manifest->heap_ptr / NACL_PAGESIZE == user_space->page_num);
  assert(nap->mem_map.is_sorted != 1);

  /* protect dead space */
  dead_space = NaClVmmapFindMaxFreeSpace(&nap->mem_map, 1) * NACL_PAGESIZE;
  i = (user_space->page_num + user_space->npages) * NACL_PAGESIZE;
  dead_space = NaClCommonSysMmapIntern(nap, (void*)i, dead_space, 0, 0x22, -1, 0);
  COND_ABORT(dead_space != i, "cannot protect dead space");

  /* force sort to remove deleted blocks */
  user_space->removed = 1;
  nap->mem_map.is_sorted = 0;
  NaClVmmapMakeSorted(&nap->mem_map);
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
  environment = GetValueByKey("Environment");
  if(environment == NULL) return;

  /* parse and check count of attributes */
  count = ParseValue(environment, ", \t", tokens, BIG_ENOUGH_SPACE);
  COND_ABORT(count % 2 != 0, "odd number of user environment variables");
  COND_ABORT(count == 0, "invalid user environment");
  COND_ABORT(count == BIG_ENOUGH_SPACE, "user environment exceeded the limit");

  /* allocate space to hold string pointers */
  count >>= 1;
  policy->envp = calloc(count + 1, sizeof(*policy->envp));

  /* construct array of environment variables */
  for(i = 0; i < count; ++i)
  {
    char *key = *tokens++;
    char *value = *tokens++;
    int length = strlen(key) + strlen(value) + 1 + 1; /* + '=' + '\0' */

    policy->envp[i] = calloc(length + 1, sizeof(*policy->envp[0]));
    COND_ABORT(policy->envp[i] == NULL, "cannot allocate memory for custom attribute");
    sprintf(policy->envp[i], "%s=%s", key, value);
  }
}

/* helper. sets command line parameters for the user */
static void SetCommandLine(struct SystemManifest *policy)
{
  int i;
  char *parameters;
  char *buf[BIG_ENOUGH_SPACE], **tokens = buf;
  char *pgm_name = GetValueByKey("NodeName");

  assert(policy != NULL);
  assert(policy->cmd_line == NULL);
  assert(policy->cmd_line_size == 0);

  /* get parameters */
  parameters = GetValueByKey("CommandLine");

  /* if there is command line parse and check count of parameters */
  if(parameters != NULL)
  {
    policy->cmd_line_size = ParseValue(parameters, " \t", tokens, BIG_ENOUGH_SPACE);
    COND_ABORT(policy->cmd_line_size == 0, "invalid user parameters");
    COND_ABORT(policy->cmd_line_size == BIG_ENOUGH_SPACE,
        "user command line parameters exceeded the limit");
  }

  /*
   * allocate space to hold string pointers. 0st element reserved
   * for argv[0]. last element should be NULL so 1 extra element must
   * be reserved
   */
  ++policy->cmd_line_size;
  policy->cmd_line = calloc(policy->cmd_line_size + 1, sizeof *policy->cmd_line);
  COND_ABORT(policy->cmd_line == NULL,
      "cannot allocate memory to hold user command line parameters");

  /* set the node name (0st parameter) */
  policy->cmd_line[0] = (pgm_name == NULL) ? NEXE_PGM_NAME : pgm_name;

  /* populate command line arguments array with pointers */
  for(i = 0; i < policy->cmd_line_size - 1; ++i)
    policy->cmd_line[i + 1] = tokens[i];

  assert(policy->cmd_line[policy->cmd_line_size] == NULL);
}

/*
 * construct system_manifest object and initialize from manifest
 * todo(d'b): everything about 'report' should be moved to HostManifestCtor()
 */
void SystemManifestCtor(struct NaClApp *nap)
{
  int32_t size;
  struct SystemManifest *policy;

  /* check for design errors */
  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  policy = nap->system_manifest;

  /* get zerovm settings from manifest */
  policy->version = GetValueByKey("Version");
  policy->nexe = GetValueByKey("Nexe");
  policy->nexe_etag = GetValueByKey("NexeEtag");
  GET_INT_BY_KEY(policy->nexe_max, "NexeMax");
  GET_INT_BY_KEY(policy->timeout, "Timeout");

  /* check mandatory manifest keys */
  COND_ABORT(nap->system_manifest->version == NULL,
      "the manifest version is not provided");
  COND_ABORT(STRCMP(nap->system_manifest->version, MANIFEST_VERSION),
      "not supported manifest version");
  size = GetFileSize(nap->system_manifest->nexe);
  COND_ABORT(size < 0, "invalid nexe");
  COND_ABORT(nap->system_manifest->nexe == NULL, "nexe name is not provided");
  if(nap->system_manifest->nexe_max) COND_ABORT(
      nap->system_manifest->nexe_max < size, "nexe file is larger then alowed");

  /* limits */
  GET_INT_BY_KEY(policy->max_mem, "MemMax");
  GET_INT_BY_KEY(policy->max_syscalls, "SyscallsMax");

  /* get the timeout an install if specified */
  GET_INT_BY_KEY(nap->system_manifest->timeout, "Timeout");
  if(nap->system_manifest->timeout != 0)
  {
    struct rlimit rl;
    rl.rlim_cur = nap->system_manifest->timeout;
    setrlimit (RLIMIT_CPU, &rl);
  }

  /* counters */
  policy->cnt_syscalls = 0;
  policy->syscallback = 0;

  /* user data (environment, command line) */
  policy->envp = NULL;
  SetCustomAttributes(policy);

  /*
   * prepare command line arguments for nexe
   * todo(d'b): allow passing '=' ',' and ' '
   */
  policy->cmd_line = NULL;
  policy->cmd_line_size = 0;
  SetCommandLine(policy);

  /* construct and initialize all channels */
  ChannelsCtor(nap);

  /*
   * allocate "whole memory chunk" if specified. should be the last allocation
   * in raw because after chunk allocated there will be no free user memory
   * note: will set "heap_ptr"
   */
  PreallocateUserMemory(nap);

  /* zerovm return code */
  nap->system_manifest->ret_code = OK_CODE;
}

/*
 * deallocate memory, close files, free other resources.
 * note: can be invoked from any place
 */
int SystemManifestDtor(struct NaClApp *nap)
{
  assert(nap != NULL);

  if(nap->system_manifest != NULL)
    ChannelsDtor(nap);

  return OK_CODE;
}

/*
 * proxy awaits zerovm report from stdout
 * only signal safe functions should be used (printf is not safe)
 * but looks like snprintf() is safe
 * todo(d'b): add timeout detection
 */
int ProxyReport(struct NaClApp *nap)
{
  char buf[BIG_ENOUGH_SPACE + 1], *report = buf;

  snprintf(report, BIG_ENOUGH_SPACE,
      "user return code = %d\nZeroVM return code = %d\nuser state = %s\n",
      nap->system_manifest->user_ret_code, nap->zvm_code, nap->zvm_state);

  write(STDOUT_FILENO, report, strlen(report));
  return OK_CODE;
}
