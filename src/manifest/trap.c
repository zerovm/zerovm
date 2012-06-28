/*
 * "single syscall" engine
 * this is the replacement for nacl multiple syscalls. however we still use
 * trampoline engine to translate invocations to system from user space
 *
 *  Created on: Dec 2, 2011
 *      Author: d'b
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <assert.h>

#include "src/manifest/trap.h"
#include "src/manifest/manifest_parser.h"
#include "src/manifest/manifest_setup.h"
#include "src/platform/nacl_log.h"
#include "api/zvm.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/platform/nacl_exit.h"

/*YaroslavLitvinov*/
#ifdef NETWORKING
#  include "src/networking/zvm_netw.h"
#  include "src/networking/zmq_netw.h" /* SockCapability */
#endif

/*
 * check number of trap() calls and increment by 1. update
 * system_manifest. return 0 if success, -1 if over limit
 */
int UpdateSyscallsCount(struct NaClApp *nap)
{
  int retcode = ERR_CODE;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  if(nap->system_manifest->cnt_syscalls < nap->system_manifest->max_syscalls)
  {
    ++nap->system_manifest->cnt_syscalls;
    retcode = OK_CODE;
  }

  return retcode;
}

/*
 * check if given counter not overrun the limit and increment by 1.
 * update system_manifest. return 0 if success, -1 if over limit
 * todo(d'b): this function is useless. remove it.
 */
int UpdateIOCounter(struct NaClApp *nap, enum ChannelType ch, enum IOCounters cntr)
{
  int retcode = ERR_CODE;
  struct ChannelDesc *channel;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert((void*)nap->system_manifest->channels != NULL);

  channel = &((struct ChannelDesc*)nap->system_manifest->channels)[ch];

  if(channel->counters[cntr] < channel->limits[cntr])
  {
    ++channel->counters[cntr];
    retcode = OK_CODE;
  }

  return retcode;
}

/* user exit. invokes long jump to main(). uses global var */
static int32_t TrapExitHandle(struct NaClApp *nap, int32_t code)
{
  assert(nap != NULL);

  NaClLog(1, "Exit syscall handler: %d\n", code);
  nap->exit_status = code;
  longjmp(user_exit, code);

  return code; /* prevent compiler warning. not reached */
}

/*
 * validate and set syscallback (both local and global)
 * return 0 if syscallback installed, otherwise -1
 *
 * update: please note, syscallback is same far jump. so it
 * must be tested the same way. we cannot use validator to
 * prevent potential danger. therefore the code below must be *very* safe
 * and must check all possibilities.
 */
static int32_t UpdateSyscallback(struct NaClApp *nap, struct UserManifest *hint)
{
  int32_t addr;
  int32_t retcode = ERR_CODE;

  assert(nap != NULL);
  assert(hint != NULL);
  assert(nap->system_manifest != NULL);

  /* uninstall syscallback if 0 given */
  syscallback = 0; /* global variable */
  nap->system_manifest->syscallback = 0;
  addr = hint->syscallback;
  if(addr == 0) return OK_CODE;

  /* check alignement */
  if(addr & (OP_ALIGNEMENT - 1)) return ERR_CODE;

  /* check if syscallback points to static text */
  retcode &=
      (uintptr_t)addr >= nap->dynamic_text_start &&
      (uintptr_t)addr < nap->dynamic_text_end ?
          OK_CODE : ERR_CODE;

  /* check if syscallback points to dynamic text */
  retcode &=
      (uintptr_t)addr >= NACL_TRAMPOLINE_END &&
      (uintptr_t)addr < nap->static_text_end ?
          OK_CODE : ERR_CODE;

  /* set the new syscallback if found in the proper place */
  if(retcode == OK_CODE)
  {
    nap->system_manifest->syscallback = addr;
    syscallback = NaClUserToSys(nap, (intptr_t) addr);
  }

  return retcode;
}

/*
 * return 0 if channel allowed to read
 * todo(NETWORKING): update with network channels
 */
static int ChannelCanBeRead(struct NaClApp *nap, enum ChannelType ch)
{
  struct ChannelDesc *channel;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert((void*)nap->system_manifest->channels != NULL);

  channel = &((struct ChannelDesc*)nap->system_manifest->channels)[ch];

  if(channel->mounted != LOADED) return ERR_CODE;
  return ch < InputChannel && ch >= nap->system_manifest->channels_count ?
      ERR_CODE : OK_CODE;
}

/*
 * return 0 if channel allowed to write
 * todo(NETWORKING): update with network channels
 */
static int ChannelCanBeWritten(struct NaClApp *nap, enum ChannelType ch)
{
  struct ChannelDesc *channel;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert((void*)nap->system_manifest->channels != NULL);

  channel = &((struct ChannelDesc*)nap->system_manifest->channels)[ch];

  if(channel->mounted != LOADED) return ERR_CODE;
  if(ch < InputChannel && ch >= nap->system_manifest->channels_count)
    return ERR_CODE;
  return ch == InputChannel ? ERR_CODE : OK_CODE;
}

/*
 * read specified amount of bytes from given desc/offset to buffer
 * return amount of read bytes or negative error code if call failed
 */
int32_t TrapReadHandle(struct NaClApp *nap,
    enum ChannelType ch, char *buffer, int32_t size, int64_t offset)
{
  struct ChannelDesc *channel;
  int64_t tail;
  char *sys_buffer;
  int32_t retcode;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert((void*)nap->system_manifest->channels != NULL);

  /* convert address and check buffer */
  sys_buffer = (char*)NaClUserToSys(nap, (uintptr_t) buffer);

  NaClLog(LOG_INFO, "%s() invoked: desc=%d, buffer=0x%lx, size=%d, offset=%ld\n",
      __func__, ch, (intptr_t)buffer, size, offset);

  /* todo(NETWORKING): integrate it into the channels */
#ifdef NETWORKING
  /*YaroslavLitvinov*/
  if(GetValueByKey("Networking") != NULL)
  {
	  int capab = ENOTALLOWED;
	  capab = capabilities_for_file_fd(ch);
	  if ( capab == EREAD || capab == EREADWRITE ){
		  retcode = commf_read(ch, sys_buffer, size);
		  if ( -1 == retcode  ){
			  NaClLog(LOG_ERROR, "%s() read file %d error\n", __func__, ch );
			  NaClAbort();
		  }
		  return retcode;
	  }
	  else{
		  /*todo read errcode into report log*/
		  NaClLog(LOG_ERROR, "%s() read file %d not allowed by capabilities err=%d\n", __func__, ch, capab );
		  NaClAbort();
	  }
  }
#endif

  if(ChannelCanBeRead(nap, ch) != OK_CODE) return -INVALID_DESC;
  channel = &((struct ChannelDesc*)nap->system_manifest->channels)[ch];

  /* check arguments sanity */
  if(size < 1) return -INSANE_SIZE;
  if(offset < 0) return -INSANE_OFFSET;

  /* check limits. update counters */
  if(offset >= channel->fsize) return -OUT_OF_BOUNDS;
  if(channel->counters[GetsCounter] >= channel->limits[GetsLimit])
    return -OUT_OF_LIMITS;
  tail = channel->limits[GetSizeLimit] - channel->counters[GetSizeCounter];
  if(size > tail) size = tail;
  if(size < 1) return -OUT_OF_LIMITS;
  ++channel->counters[GetsCounter];
  channel->counters[GetSizeCounter] += size;

  /* read data */
  retcode = pread(channel->handle, sys_buffer, (size_t)size, (off_t)offset);

  return retcode;
}

/*
 * write specified amount of bytes from buffer to given desc/offset
 * return amount of read bytes or negative error code if call failed
 */
int32_t TrapWriteHandle(struct NaClApp *nap,
    enum ChannelType ch, char *buffer, int32_t size, int64_t offset)
{
  struct ChannelDesc *channel;
  int64_t tail;
  char *sys_buffer;
  int32_t retcode;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert((void*)nap->system_manifest->channels != NULL);

  /* convert address and check buffer */
  sys_buffer = (char*)NaClUserToSys(nap, (uintptr_t) buffer);

  NaClLog(4, "%s() invoked: desc=%d, buffer=0x%lx, size=%d, offset=%ld\n",
        __func__, ch, (intptr_t)buffer, size, offset);

  /* todo(NETWORKING): integrate it into the channels */
#ifdef NETWORKING
  /*YaroslavLitvinov*/
  if(GetValueByKey("Networking") != NULL)
  {
	  int capab = ENOTALLOWED;
	  capab = capabilities_for_file_fd(ch);
	  if ( capab == EWRITE || capab == EREADWRITE ){
		  retcode = commf_write(ch, sys_buffer, size);
		  if ( -1 == retcode  ){
			  NaClLog(LOG_ERROR, "%s() write file %d error\n", __func__, ch );
			  NaClAbort();
		  }
		  return retcode;
	  }
	  else{
		  /*todo write errcode into report log*/
		  NaClLog(LOG_ERROR, "%s() write file %d not allowed by capabilities err=%d\n", __func__, ch, capab );
		  NaClAbort();
	  }
  }
#endif

  if(ChannelCanBeWritten(nap, ch) != OK_CODE) return -INVALID_DESC;
  channel = &((struct ChannelDesc*)nap->system_manifest->channels)[ch];

  /* check arguments sanity */
  if(size < 1) return -INSANE_SIZE;
  if(offset < 0) return -INSANE_OFFSET;

  /* check limits. update counters */
  if(offset >= channel->fsize) return -OUT_OF_BOUNDS;
  if(channel->counters[PutsCounter] >= channel->limits[PutsLimit])
    return -OUT_OF_LIMITS;
  tail = channel->limits[PutSizeLimit] - channel->counters[PutSizeCounter];
  if(size > tail) size = tail;
  if(size < 1) return -OUT_OF_LIMITS;
  ++channel->counters[PutsCounter];
  channel->counters[PutSizeCounter] += size;

  /* write data */
  retcode = pwrite(channel->handle, sys_buffer, (size_t)size, (off_t)offset);

  return retcode;
}

/*
 * put allowed information from the system manifest to provided
 * object of struct UserManifest and update syscallback
 * return 0 if syscallback was updated, otherwise negative error code
 * todo(d'b): extract syscallback setting to separate function
 */
static int32_t TrapUserSetupHandle(struct NaClApp *nap, struct UserManifest *h)
{
  struct SystemManifest *policy;
  struct UserManifest *hint;
  int32_t retcode = OK_CODE;
  enum ChannelType ch;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert((void*)nap->system_manifest->channels != NULL);

  /* check provided object */
  if(h == NULL) return ERR_CODE;
  hint = (struct UserManifest*)NaClUserToSys(nap, (intptr_t) h);
  if((void*)hint->channels == NULL) return ERR_CODE;

  /* copy and translate information from system manifest */
  policy = nap->system_manifest;
  hint->cnt_syscalls = policy->cnt_syscalls;
  memcpy(hint->content_type, policy->content_type, CONTENT_TYPE_LEN);
  hint->heap_ptr = policy->heap_ptr;
  hint->max_mem = policy->max_mem;
  hint->max_syscalls = policy->max_syscalls;
  hint->self_size = sizeof(*hint);
  memcpy(hint->timestamp, policy->timestamp, TIMESTAMP_LEN);
  memcpy(hint->user_etag, policy->user_etag, USER_TAG_LEN);
  memcpy(hint->x_object_meta_tag, policy->x_object_meta_tag, X_OBJECT_META_TAG_LEN);

  /* secure set of channels information */
  for(ch = InputChannel; ch < nap->system_manifest->channels_count; ++ch)
  {
    /* pick current channel settings */
    struct ChannelDesc *channel = &((struct ChannelDesc*)policy->channels)[ch];
    struct ChannelDesc *hint_channel = (struct ChannelDesc*)
        NaClUserToSys(nap, (uintptr_t)&((struct ChannelDesc*)hint->channels)[ch]);

    /* skip empty channels but put the mark that channel does not exist */
    hint_channel->type = EmptyChannel;
    if(channel->name == (int64_t)NULL) continue;

    hint_channel->bsize = channel->bsize;
    hint_channel->buffer = channel->buffer;
    memcpy(hint_channel->counters, channel->counters,
        IOCountersCount * sizeof(*hint_channel->counters));
    hint_channel->fsize = channel->fsize;
    hint_channel->handle = ch;
    memcpy(hint_channel->limits, channel->limits,
        IOLimitsCount * sizeof(*hint_channel->limits));
    hint_channel->mounted = channel->mounted;
    hint_channel->name = 0; /* NULL. todo(d'b): give user standard channel name */
    hint_channel->self_size = sizeof(*hint_channel);
    hint_channel->type = channel->type;
  }

  /* update syscallback */
  if(UpdateSyscallback(nap, hint) == ERR_CODE) retcode = ERR_CODE;

  return retcode;
}

/*
 * "One Ring" syscall main routine. the nacl syscalls replacement.
 * "args" is an array of syscall name and its arguments
 * note: since nacl patch two 1st arguments if they are pointers, arg[1] are not used
 * todo(d'b): check how nacl decide to patch arguments.
 */
int32_t TrapHandler(struct NaClApp *nap, uint32_t args)
{
  uint64_t *sys_args;
  int retcode = 0;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /*
   * translate address from user space to system
   * note: cannot set "trap error"
   */
  sys_args = (uint64_t*)NaClUserToSys(nap, (uintptr_t) args);
  NaClLog(4, "Trap arguments address = 0x%lx\n", (intptr_t)sys_args);

  switch(*sys_args)
  {
    case TrapExit:
      retcode = TrapExitHandle(nap, (int32_t) sys_args[2]);
      break;
    case TrapUserSetup:
      retcode = TrapUserSetupHandle(nap, (struct UserManifest*) sys_args[2]);
      break;
    case TrapRead:
      retcode = TrapReadHandle(nap,
          (enum ChannelType)sys_args[2], (char*)sys_args[3], (int32_t)sys_args[4], sys_args[5]);
      break;
    case TrapWrite:
      retcode = TrapWriteHandle(nap,
          (enum ChannelType)sys_args[2], (char*)sys_args[3], (int32_t)sys_args[4], sys_args[5]);
      break;
    default:
      retcode = ERR_CODE;
      NaClLog(LOG_ERROR, "function %ld is not supported\n", *sys_args);
      break;
  }

  NaClLog(4, "leaving Trap with code = 0x%x\n", retcode);
  return retcode;
}

