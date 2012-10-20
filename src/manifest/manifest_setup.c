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
#include <sys/mman.h>
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/etag.h"
#include "src/service_runtime/nacl_syscall_handlers.h"
#include "src/service_runtime/sel_memory.h"
#include "src/manifest/manifest_parser.h"
#include "src/manifest/manifest_setup.h"
#include "src/manifest/mount_channel.h"

/* lower zerovm priority */
static void LowerOwnPriority()
{
  ZLOGIF(setpriority(PRIO_PROCESS, 0, ZEROVM_PRIORITY) != 0,
      "cannot lower zerovm priority");
}

/* put zerovm in a "jail" */
static void ChrootJail()
{
  ZLOGS(LOG_DEBUG, "'chrooting' zerovm to %s", NEW_ROOT);
  ZLOGIF(chdir(NEW_ROOT) != 0, "cannot 'chdir' zerovm");
  ZLOGIF(chroot(NEW_ROOT) != 0, "cannot 'chroot' zerovm");
}

/* limit zerovm i/o */
static void LimitOwnIO(struct NaClApp *nap)
{
  struct rlimit rl;

  assert(nap != NULL);
  assert(nap->storage_limit > 0);

  ZLOGIF(getrlimit(RLIMIT_FSIZE, &rl) != 0, "cannot get i/o limits");
  rl.rlim_cur = nap->storage_limit;
  ZLOGIF(setrlimit(RLIMIT_FSIZE, &rl) != 0, "cannot set i/o limits");
}

/* limit heap and stack available for zerovm */
/* todo(d'b): under construction */
static void LimitOwnMemory()
{
#if 0
  int result;

  ZLOGIF(result != 0, "cannot limit zerovm i/o");
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
  ZLOGIF(result != 0, "cannot disable super user");
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
  uint32_t heap;
  void *p;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /* quit function if max_mem is not specified in manifest */
  ZLOGFAIL(nap->heap_end == 0, EFAULT, "user heap size in not specified in manifest");

  /* calculate user heap size (must be allocated next to the data_end) */
  p = (void*)NaClRoundAllocPage(nap->data_end);
  heap = nap->heap_end - nap->stack_size;
  heap = NaClRoundAllocPage(heap) - NaClRoundAllocPage(nap->data_end);

  /* since 4gb of user space is already allocated just set protection to the heap */
  p = (void*)NaClUserToSys(nap, (uintptr_t)p);
  i = NaCl_mprotect(p, heap, PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != i, -i, "cannot set protection on user heap");
  nap->heap_end = (uintptr_t)p + heap;
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
  ZLOGFAIL(count % 2 != 0, EFAULT, "odd number of user environment variables");
  ZLOGFAIL(count == 0, EFAULT, "invalid user environment");
  ZLOGFAIL(count == BIG_ENOUGH_SPACE, EFAULT, "user environment exceeded the limit");

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
    ZLOGFAIL(policy->envp[i] == NULL, ENOMEM, "cannot allocate memory for custom attribute");
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
    ZLOGFAIL(i != 2, EFAULT, "invalid NodeName specified");
    ZLOGFAIL(tokens[0] == NULL, EFAULT, "invalid node name");
    ZLOGFAIL(tokens[1] == NULL, EFAULT, "invalid node id");
    nap->system_manifest->cmd_line[0] = tokens[0];
    nap->node_id = ATOI(tokens[1]);
    ZLOGFAIL(nap->node_id == 0, EFAULT, "node id must be > 0");
  }

  /* put node name and id to the log */
  ZLOGS(LOG_DEBUG, "node name = %s, node id = %d",
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
    ZLOGFAIL(policy->cmd_line_size == 0, EFAULT, "invalid user parameters");
    ZLOGFAIL(policy->cmd_line_size == BIG_ENOUGH_SPACE, EFAULT, "too long user command line");
  }

  /*
   * allocate space to hold string pointers. 0st element reserved for argv[0].
   * also, the last element should be NULL so 1 extra element must be reserved
   */
  ++policy->cmd_line_size;
  policy->cmd_line = calloc(policy->cmd_line_size + 1, sizeof *policy->cmd_line);
  ZLOGFAIL(policy->cmd_line == NULL, ENOMEM,
      "cannot allocate memory for user command line parameters");

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
  ZLOGFAIL(policy->timeout < 1, EFAULT, "invalid or absent timeout");
  rl.rlim_cur = policy->timeout;
  rl.rlim_max = -1;
  setrlimit(RLIMIT_CPU, &rl);
  ZLOGFAIL(setrlimit(RLIMIT_CPU, &rl) != 0, errno, "cannot set timeout");
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

  /*
   * prepare overall etag context. if failed report will not be shown
   * todo(d'b): show report
   */
  if(TagEngineEnabled())
  {
    if(TagCtor(&nap->user_tag) == ERR_CODE)
    {
      ZLOG(LOG_ERROR, "cannot construct overall channels tag");
      _exit(1);
    }
  }

  /* check mandatory manifest keys */
  ZLOGFAIL(nap->system_manifest->version == NULL, EFAULT,
      "the manifest version is not provided");
  ZLOGFAIL(STRCMP(nap->system_manifest->version, MANIFEST_VERSION),
      EFAULT, "manifest version not supported");
  ZLOGFAIL(nap->system_manifest->nexe == NULL, EFAULT, "nexe name not provided");
  size = GetFileSize(nap->system_manifest->nexe);
  ZLOGFAIL(size < 0, EIO, "invalid nexe");
  if(nap->system_manifest->nexe_max)
    ZLOGFAIL(nap->system_manifest->nexe_max < size, EFBIG, "nexe size = %d", size);
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
  GET_INT_BY_KEY(nap->heap_end, "MemMax");
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

/* updates user_tag (should be constructed) with memory chunk data */
static void EtagMemoryChunk(void *state, struct NaClVmmapEntry *vmep)
{
  uintptr_t addr;
  int32_t size;
  struct NaClApp *nap = state;
  int i;

  assert(nap != NULL);
  assert(TagEngineEnabled() != 0);

  /* skip inaccessible and removed pages */
  if(vmep->prot == 0) return;
  if(vmep->removed) return;

  /* update user_etag with the chunk data */
  addr = nap->mem_start + (vmep->page_num << NACL_PAGESHIFT);
  size = vmep->npages << NACL_PAGESHIFT;
  i = TagUpdate(&nap->user_tag, (const char*)addr, size);
  ZLOGIF(i == ERR_CODE, "cannot update user_tag");
}

/* updates user_tag (should be constructed) with all channels digests */
void ChannelsDigest(struct NaClApp *nap)
{
  int i;
  int code;

  assert(nap != NULL);

  /* go through all channels */
  for(i = 0; i < nap->system_manifest->channels_count; ++i)
  {
    code = TagUpdate(&nap->user_tag,
        (const char*)nap->system_manifest->channels[i].digest, TAG_DIGEST_SIZE);
    ZLOGIF(code == ERR_CODE, "cannot update overall channels tag");
  }
}

/*
 * proxy awaits zerovm report from stdout
 * only signal safe functions should be used (printf is not safe)
 * but looks like snprintf() is safe
 */
int ProxyReport(struct NaClApp *nap)
{
  char report[BIG_ENOUGH_SPACE + 1];
  char etag[TAG_DIGEST_SIZE] = TAG_ENGINE_DISABLED;
  int length;
  int i;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /* tag user memory and channels */
  /* bug: if user memory is not set up zvm crashes */
  if(TagEngineEnabled())
  {
    ChannelsDigest(nap);
    NaClVmmapVisit(&nap->mem_map, EtagMemoryChunk, nap);
    TagDigest(nap->user_tag, etag);
  }

  /* for debugging purposes it is useful to see more advanced information */
#ifdef DEBUG
  length = snprintf(report, BIG_ENOUGH_SPACE,
      "validator state = %d\nuser return code = %d\netag = %s\naccounting = %s\n"
      "exit state = %s\n", nap->validation_state,
      nap->system_manifest->user_ret_code, etag, nap->accounting, nap->zvm_state);
#else
  /* .. but for production zvm will switch to more brief output */
  length = snprintf(report, BIG_ENOUGH_SPACE, "%d\n%d\n%s\n%s\n%s\n",
      nap->validation_state, nap->system_manifest->user_ret_code,
      etag, nap->accounting, nap->zvm_state);
#endif

  /* give the report to proxy */
  i = write(STDOUT_FILENO, report, length);

  /* log the report */
  length = snprintf(report, BIG_ENOUGH_SPACE,
      "validator state = %d, user return code = %d, etag = %s, accounting = %s, "
      "exit state = %s", nap->validation_state,
      nap->system_manifest->user_ret_code, etag, nap->accounting, nap->zvm_state);
  ZLOGS(LOG_DEBUG, "%s", report);

  return i == length ? OK_CODE : ERR_CODE;
}
