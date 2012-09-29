/*
 * construct system manifest and host manifest
 * system/channels resources counters are also here
 *
 *  Created on: Nov 30, 2011
 *      Author: d'b
 */
#include <assert.h>
#include <time.h>
#include <sys/resource.h> /* timeout, process priority */
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/etag.h"
#include "src/service_runtime/nacl_syscall_handlers.h"
#include "src/manifest/manifest_parser.h"
#include "src/manifest/manifest_setup.h"
#include "src/manifest/mount_channel.h"

/* lower zerovm priority */
static void LowerOwnPriority()
{
  if(setpriority(PRIO_PROCESS, 0, ZEROVM_PRIORITY) != 0)
    NaClLog(LOG_ERROR, "cannot lower zerovm priority");
}

/* put zerovm in a "jail" */
static void ChrootJail()
{
  if(chdir(NEW_ROOT) != 0)
    NaClLog(LOG_ERROR, "cannot 'chdir' zerovm");

  if(chroot(NEW_ROOT) != 0)
    NaClLog(LOG_ERROR, "cannot 'chroot' zerovm");
}

/* limit zerovm i/o */
/* todo(d'b): calculate limit from channels */
static void LimitOwnIO(struct NaClApp *nap)
{
  struct rlimit rl;

  assert(nap != NULL);
  assert(nap->storage_limit > 0);

  if(getrlimit(RLIMIT_FSIZE, &rl) != 0)
    NaClLog(LOG_ERROR, "cannot get i/o limits");
  rl.rlim_cur = nap->storage_limit;
  if(setrlimit(RLIMIT_FSIZE, &rl) != 0)
    NaClLog(LOG_ERROR, "cannot set i/o limits");
}

/* limit heap and stack available for zerovm */
/* todo(d'b): under construction */
static void LimitOwnMemory()
{
#if 0
  int result;

  if(result != 0)
    NaClLog(LOG_ERROR, "cannot limit zerovm i/o");
#endif
}

/* give up the super user priority */
/* todo(d'b): under construction */
static void DisableSuperUser()
{
#if 0
  int result;

  if (getuid() == 0)
  {
    /* process is running as root, drop privileges */
    if (setgid(groupid) != 0)
        fatal("setgid: Unable to drop group privileges: %s", strerror(errno));
    if (setuid(userid) != 0)
        fatal("setuid: Unable to drop user privileges: %S", strerror(errno));
  }

  // ###
  if(result != 0)
    NaClLog(LOG_ERROR, "cannot disable super user");
#endif
}

/*
 * "defense in depth". the last frontier of defense.
 * zerovm limits itself as much as possible
 */
void LastDefenseLine(struct NaClApp *nap)
{
  LowerOwnPriority();
  LimitOwnIO(nap);
  LimitOwnMemory();
  ChrootJail();
  DisableSuperUser();
}

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

/* sets node_id in nap object and user argv[0] */
static void SetNodeName(struct NaClApp *nap)
{
  int i;
  char *buf[BIG_ENOUGH_SPACE], **tokens = buf;
  char *pgm_name = GetValueByKey("NodeName");

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->system_manifest->cmd_line != NULL);
  assert(nap->system_manifest->cmd_line_size > 0);

  /* set the node name (0st parameter) and node id (n/a for the user) */
  if(pgm_name == NULL)
  {
    nap->system_manifest->cmd_line[0] = NEXE_PGM_NAME;
    nap->node_id = 0;
  }
  else
  {
    i = ParseValue(pgm_name, ",", tokens, BIG_ENOUGH_SPACE);
    COND_ABORT(i != 2, "invalid NodeName specified");
    COND_ABORT(tokens[0] == NULL, "invalid node name");
    COND_ABORT(tokens[1] == NULL, "invalid node id");
    nap->system_manifest->cmd_line[0] = tokens[0];
    nap->node_id = ATOI(tokens[1]);
    COND_ABORT(nap->node_id == 0, "node id must be > 0");
  }

  /* put node name and id to the log */
  NaClLog(LOG_DEBUG, "node name = %s, node id = %d",
      nap->system_manifest->cmd_line[0], nap->node_id);
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
   * allocate space to hold string pointers. 0st element reserved for argv[0].
   * also, the last element should be NULL so 1 extra element must be reserved
   */
  ++policy->cmd_line_size;
  policy->cmd_line = calloc(policy->cmd_line_size + 1, sizeof *policy->cmd_line);
  COND_ABORT(policy->cmd_line == NULL,
      "cannot allocate memory to hold user command line parameters");

  /* populate command line arguments array with pointers */
  for(i = 0; i < policy->cmd_line_size - 1; ++i)
    policy->cmd_line[i + 1] = tokens[i];
}

/* set timeout. by design "Timeout" must be specified in manifest */
static void SetTimeout(struct SystemManifest *policy)
{
  struct rlimit rl;

  assert(policy != NULL);

  GET_INT_BY_KEY(policy->timeout, "Timeout");
  COND_ABORT(policy->timeout < 1, "invalid or absent timeout");
  rl.rlim_cur = policy->timeout;
  setrlimit (RLIMIT_CPU, &rl);
}

/* construct system_manifest object and initialize it from manifest */
void SystemManifestCtor(struct NaClApp *nap)
{
  int32_t size;
  struct SystemManifest *policy;

  /* check for design errors */
  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  policy = nap->system_manifest;
  policy->syscallback = 0;

  /* get zerovm settings from manifest */
  policy->version = GetValueByKey("Version");
  policy->nexe = GetValueByKey("Nexe");
  policy->nexe_etag = GetValueByKey("NexeEtag");
  GET_INT_BY_KEY(policy->nexe_max, "NexeMax");

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
  SetTimeout(policy);

  /*
   * todo(d'b): command line and environment should be passed to the untrusted
   * "as is" through the dedicated channel: "/dev/environment"
   */

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
  GET_INT_BY_KEY(policy->max_mem, "MemMax");
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

/* read information for the given stat */
static int64_t ReadExtendedStat(const struct NaClApp *nap, const char *stat)
{
  char path[BIG_ENOUGH_SPACE + 1];
  char buf[BIG_ENOUGH_SPACE + 1];
  int64_t result;
  FILE *f;

  assert(nap != NULL);
  assert(stat != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->system_manifest->extended_accounting != NULL);

  /* construct the stat path to read */
  snprintf(path, BIG_ENOUGH_SPACE, "%s/%s",
      nap->system_manifest->extended_accounting, stat);

  /* open, read and close stat file */
  f = fopen(path, "r");
  if(f == NULL)
  {
    NaClLog(LOG_ERROR, "cannot open %s", path);
    return ERR_CODE;
  }
  result = fread(buf, 1, BIG_ENOUGH_SPACE, f);
  if(result == 0 || result == BIG_ENOUGH_SPACE)
  {
    NaClLog(LOG_ERROR, "error statistics reading for %s", stat);
    return ERR_CODE;
  }
  fclose(f);

  /* convert and return the result */
  return atoi(buf);
}

/* populate given string with an extended accounting statistics */
static void ReadExtendedAccounting(const struct NaClApp *nap, char *buf, int size)
{
  int64_t user_cpu = 0;
  int64_t memory_size = 0;
  int64_t swap_size = 0;

  assert(nap != NULL);
  assert(buf != NULL);
  assert(nap->system_manifest != NULL);
  assert(size > 0);

  /* get statistics if there is an extended accounting */
  if(nap->system_manifest->extended_accounting != NULL)
  {
    /* get statistics */
    user_cpu = ReadExtendedStat(nap, CGROUPS_USER_CPU); /* ### cast to ms! */
    memory_size = ReadExtendedStat(nap, CGROUPS_MEMORY);
    swap_size = ReadExtendedStat(nap, CGROUPS_SWAP);
  }

  /* construct and return the result */
  snprintf(buf, size, "%ld %ld %ld", user_cpu, memory_size, swap_size);
}

/*
 * finalize accounting if needed
 * todo(d'b): under construction
 */
static void StopExtendedAccounting(struct NaClApp *nap)
{
  int result;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /* exit if there is no extended accounting */
  if(nap->system_manifest->extended_accounting == NULL) return;

  /* todo(d'b): move itself to another "tasks" */

  /* remove own pid folder from cgroups folder */
  result = rmdir(nap->system_manifest->extended_accounting);
  if(result != 0)
  {
    NaClLog(LOG_ERROR, "cannot remove %s",
      nap->system_manifest->extended_accounting);
    return;
  }

  /* anything else?.. */
}

/*
 * collect accounting statistics about used cpu, memory and storage
 * and put it to provided buffer. if the buffer size is not big enough
 * drops the rest.
 * todo(): under construction
 */
static void GatherStatistics(struct NaClApp *nap, char *buf, int size)
{
  int i;
  int64_t network_stats[IOLimitsCount] = {0};
  int64_t local_stats[IOLimitsCount] = {0};
  int64_t real_cpu = 0;
  char external_stats[BIG_ENOUGH_SPACE + 1];

  assert(nap != NULL);
  assert(buf != NULL);
  assert(nap->system_manifest != NULL);
  assert(size != 0);

  /* gather all channels statistics */
  for(i = 0; i < nap->system_manifest->channels_count; ++i)
  {
    struct ChannelDesc *channel = &nap->system_manifest->channels[i];
    int64_t *stats;
    int j;

    assert(channel->source == LocalFile || channel->source == NetworkChannel);
    stats = channel->source == LocalFile ? local_stats : network_stats;

    for(j = 0; j < IOLimitsCount; ++j)
      stats[j] += channel->counters[j];
  }

  /* read extended information */
  ReadExtendedAccounting(nap, external_stats, BIG_ENOUGH_SPACE);
  StopExtendedAccounting(nap);
  real_cpu = (int64_t)(clock() / (double)(CLOCKS_PER_SEC / 1000));

  /* construct the accounting statistics string */
  snprintf(buf, size, "%ld %s %ld %ld %ld %ld %ld %ld %ld %ld",
      real_cpu, external_stats, /* extended information */
      local_stats[GetsLimit], local_stats[GetSizeLimit], /* local channels input */
      local_stats[PutsLimit], local_stats[PutSizeLimit], /* local channels output */
      network_stats[GetsLimit], network_stats[GetSizeLimit], /* network channels input */
      network_stats[PutsLimit], network_stats[PutSizeLimit]);  /* network channels output */
}

/* update overall etag with memory chunk */
static void EtagMemoryChunk(void *state, struct NaClVmmapEntry *vmep)
{
  SHA_CTX ctx;
  uintptr_t addr;
  int32_t size;
  struct NaClApp *nap = state;
  const char *hex;

  UNREFERENCED_PARAMETER(state);
  assert(nap != NULL);
  assert(EtagEnabled() != 0);

  /* skip inaccessible and removed pages */
  if(vmep->prot == 0) return;
  if(vmep->removed) return;

  /* construct the chunk hash context */
  if(ConstructCTX(&ctx) == ERR_CODE)
  {
    ErrIf(1, "error initializing memory chunk etag");
    return;
  }

  /* get etag for the chunk */
  addr = nap->mem_start + (vmep->page_num << NACL_PAGESHIFT);
  size = vmep->npages << NACL_PAGESHIFT;
  hex = UpdateEtag(&ctx, (const char*)addr, size);
  NaClLog(LOG_DEBUG, "memory chunk addr = %lx, size = %d, etag = %s",
      addr - nap->mem_start, size, hex);

  /* update overall etag with the chunk one */
  OverallEtag(&ctx);
}

/*
 * proxy awaits zerovm report from stdout
 * only signal safe functions should be used (printf is not safe)
 * but looks like snprintf() is safe
 */
int ProxyReport(struct NaClApp *nap)
{
  char report[BIG_ENOUGH_SPACE + 1];
  char accounting[BIG_ENOUGH_SPACE + 1];
  const char *etag = ETAG_DISABLED;
  int length;
  int i;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  GatherStatistics(nap, accounting, BIG_ENOUGH_SPACE);

  /* etag user memory */
  if(EtagEnabled())
  {
    NaClVmmapVisit(&nap->mem_map, EtagMemoryChunk, nap);
    etag = EtagToText((unsigned char*)OverallEtag(NULL));
  }

  /* for debugging purposes it is useful to see more advanced information */
#ifdef DEBUG
  length = snprintf(report, BIG_ENOUGH_SPACE,
      "validator state = %d\nuser return code = %d\netag = %s\n"
      "accounting = %s\nexit state = %s\n",
      nap->validation_state, nap->system_manifest->user_ret_code,
      etag, accounting, nap->zvm_state);
#else
  /* .. but for production zvm will switch to more brief output */
  length = snprintf(report, BIG_ENOUGH_SPACE, "%d\n%d\n%s\n%s\n%s\n",
      nap->validation_state, nap->system_manifest->user_ret_code,
      etag, accounting, nap->zvm_state);
#endif

  report[length] = '\0';
  NaClLog(LOG_INFO, "%s", report);
  i = write(STDOUT_FILENO, report, length);
  return i == length ? OK_CODE : ERR_CODE;
}
