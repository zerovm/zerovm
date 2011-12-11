/*
 * "single syscall" engine
 * this is replacement for nacl multiple syscalls. however we still use
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

#include "src/manifest/trap.h"
#include "src/manifest/manifest_setup.h"
#include "src/manifest/mount_channel.h"
#include "src/platform/nacl_log.h"
#include "api/zvm_manifest.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/nacl_globals.h"

/* pause cpu time counting. update cnt_cpu */
void PauseCpuClock(struct SetupList *policy)
{
  clock_t current = clock();
  policy->cnt_cpu += current - policy->cnt_cpu_last;
  policy->cnt_cpu_last = current;
}

/* resume cpu time counting */
void ResumeCpuClock(struct SetupList *policy)
{
  policy->cnt_cpu_last = clock();
}

/*
 * validate and set syscallback (both local and global)
 * return 0 if syscallback installed, otherwise -1
 */
static int32_t UpdateSyscallback(struct NaClApp *nap, struct SetupList *hint)
{
  int32_t addr = hint->syscallback;
  int32_t retcode = ERR_CODE;
  syscallback = 0;
  nap->manifest->user_setup->syscallback = 0;

#define MAX_OP_SIZE 0x10 /* ### take it from the validator */
#define OP_ALIGNEMENT 0x10 /* ### take it from the validator */

  if(!addr) return OK_CODE; /* user wants to uninstall syscallback */

  /* check alignement */
  if(addr & (OP_ALIGNEMENT - 1)) return ERR_CODE;

  /* seek syscallback in static text (aware of right border proximity) */
  if(addr >= nap->dynamic_text_start && addr < nap->dynamic_text_end - MAX_OP_SIZE)
    retcode = OK_CODE;

  /* seek placement in dynamic text (aware of right border proximity) */
  if(addr >= NACL_TRAMPOLINE_END && addr < nap->static_text_end - MAX_OP_SIZE )
    retcode = OK_CODE;

  /* set the new syscallback if found in the proper place */
  if(retcode == OK_CODE)
  {
    nap->manifest->user_setup->syscallback = addr;
    syscallback = NaClUserToSys(nap, (intptr_t) addr);
  }

  return retcode;
}

/*
 * read specified amount of bytes from given desc/offset to buffer
 * return amount of read bytes or negative error code if call failed
 */
int32_t TrapReadHandle(struct NaClApp *nap,
    enum ChannelType desc, char *buffer, int32_t size, int64_t offset)
{
  struct PreOpenedFileDesc *fd;
  int32_t tail;
  char *sys_buffer;
  int32_t retcode;

  // ### make it function with editable list of available channels
  /* only allow this call for InputChannel, OutputChannel */
  if(desc != InputChannel && desc != OutputChannel) return -INVALID_DESC;

  /* convert address and check buffer */
  sys_buffer = (char*)NaClUserToSys(nap, (uintptr_t) buffer);

  NaClLog(4, "%s() invoked: desc=%d, buffer=0x%lx, size=%d, offset=%ld\n",
      __func__, desc, (intptr_t)buffer, size, offset);

  /* take fd from nap with given desc */
  if(nap == NULL) return -INTERNAL_ERR;
  fd = &nap->manifest->user_setup->channels[desc];
  if(fd == NULL) return -INVALID_DESC;
  if(fd->mounted != LOADED) return -INVALID_MODE; // ### make it function with editable list of available channels

  /* check arguments sanity */
  if(size < 1) return -INSANE_SIZE;
  if(offset < 0) return -INSANE_OFFSET;

  /* check/update limits/counters */
  if(offset >= fd->fsize) return -OUT_OF_BOUNDS;
  if(fd->cnt_gets >= fd->max_gets) return -OUT_OF_LIMITS;

  tail = fd->max_get_size - fd->cnt_get_size;
  if(size > tail) size = tail;
  if(size < 1) return -OUT_OF_LIMITS;

  /* update counters (even if syscall failed) */
  ++fd->cnt_gets;
  fd->cnt_get_size += size;

  /* read data */
  retcode = pread(fd->handle, sys_buffer, (size_t)size, (off_t)offset);

  return retcode;
}

/*
 * write specified amount of bytes from buffer to given desc/offset
 * return amount of read bytes or negative error code if call failed
 */
int32_t TrapWriteHandle(struct NaClApp *nap,
    enum ChannelType desc, char *buffer, int32_t size, int64_t offset)
{
  struct PreOpenedFileDesc *fd;
  int32_t tail;
  char *sys_buffer;
  int32_t retcode;

  /* only allow this call for OutputChannel */
  if(desc != OutputChannel) return -INVALID_DESC;

  /* convert address and check buffer */
  sys_buffer = (char*)NaClUserToSys(nap, (uintptr_t) buffer);

  NaClLog(4, "%s() invoked: desc=%d, buffer=0x%lx, size=%d, offset=%ld\n",
        __func__, desc, (intptr_t)buffer, size, offset);

  /* take fd from nap with given desc */
  if(nap == NULL) return -INTERNAL_ERR;
  fd = &nap->manifest->user_setup->channels[desc];
  if(fd == NULL) return -INVALID_DESC;
  if(fd->mounted != LOADED) return -INVALID_MODE; // ### make it function with editable list of available channels

  /* check arguments sanity */
  if(size < 1) return -INSANE_SIZE;
  if(offset < 0) return -INSANE_OFFSET;

  /* check/update limits/counters */
  if(offset >= fd->fsize) return -OUT_OF_BOUNDS;
  if(fd->cnt_puts >= fd->max_puts) return -OUT_OF_LIMITS;

  tail = fd->max_put_size - fd->cnt_put_size;
  if(size > tail) size = tail;
  if(size < 1) return -OUT_OF_LIMITS;

  /* update counters (even if syscall failed) */
  ++fd->cnt_puts;
  fd->cnt_put_size += size;

  /* read data */
  retcode = pwrite(fd->handle, sys_buffer, (size_t)size, (off_t)offset);

  return retcode;
}

/*
 * user request to change limits for system resources. for now we only can decrease bounds
 * return: function update given SetupList object (hint) and if there were
 * unsuccessful attempt to change policy return -1, otherwirse return 0
 */
static int32_t TrapUserSetupHandle(struct NaClApp *nap, struct SetupList *h)
{
  struct PreOpenedFileDesc *policy_channel;
  struct PreOpenedFileDesc *hint_channel;
  struct SetupList *policy = nap->manifest->user_setup;
  struct SetupList *hint = (struct SetupList*)NaClUserToSys(nap, (intptr_t) h);
  int32_t retcode = OK_CODE;
  enum ChannelType ch;

  /*
   * check/count this call. decrease number of syscalls
   * since this call is not really "system"
   */
  --nap->manifest->user_setup->cnt_syscalls;
  if(policy->max_setup_calls < ++policy->cnt_setup_calls)
    return -OUT_OF_LIMITS;

  /* check i/o limits */
  for(ch = InputChannel; ch < CHANNELS_COUNT; ++ch)
  {
    /* pick current channel settings */
    policy_channel = &policy->channels[ch];
    hint_channel = &hint->channels[ch];
    hint_channel->self_size = policy_channel->self_size; /* set self size */

#define STRNCPY_NULL(a, b, n) if ((a) && (b)) strncpy((a), (b), (n));
#define TRY_UPDATE(pretender, current)\
    do {\
      if(policy->cnt_setup_calls == 1) { pretender = current; break; }\
      if((pretender) < (current)) current = pretender;\
      else { pretender = current; retcode = ERR_CODE; }\
    } while(0)

    /* check/update i/o limits. 1st time - set fields in hint */
    TRY_UPDATE(hint_channel->max_size, policy_channel->max_size);
    TRY_UPDATE(hint_channel->max_get_size, policy_channel->max_get_size);
    TRY_UPDATE(hint_channel->max_gets, policy_channel->max_gets);
    TRY_UPDATE(hint_channel->max_put_size, policy_channel->max_put_size);
    TRY_UPDATE(hint_channel->max_puts, policy_channel->max_puts);

    /* set i/o fields n/a to change */
    hint_channel->bsize = policy_channel->bsize;
    hint_channel->buffer = policy_channel->buffer;
    hint_channel->fsize = policy_channel->fsize;
    hint_channel->type = policy_channel->type;
  }

  /* check/update system limits. 1st time - set fields in hint */
  TRY_UPDATE(hint->max_cpu, policy->max_cpu);
  TRY_UPDATE(hint->max_mem, policy->max_mem); /* ### call unmap here? */
  TRY_UPDATE(hint->max_syscalls, policy->max_syscalls);
  TRY_UPDATE(hint->max_setup_calls, policy->max_setup_calls);

  /* set system fields n/a to change */
  hint->self_size = policy->self_size; /* set self size */
  hint->heap_ptr = policy->heap_ptr; /* set self size */
  STRNCPY_NULL(hint->content_type, policy->content_type, CONTENT_TYPE_LEN);
  STRNCPY_NULL(hint->timestamp, policy->timestamp, TIMESTAMP_LEN);
  STRNCPY_NULL(hint->user_etag, policy->user_etag, USER_TAG_LEN);
  STRNCPY_NULL(hint->x_object_meta_tag, policy->x_object_meta_tag, X_OBJECT_META_TAG_LEN);

  /* update syscallback */
  if(UpdateSyscallback(nap, hint) == ERR_CODE) retcode = ERR_CODE;

#undef STRNCPY_NULL
#undef TRY_UPDATE
  return retcode;
}

/*
 * "One Ring" syscall main routine
 *
 * 1st parameter is a pointer to the command (function, arg1, reserved, argv3,..)
 * return int32_t, value depends on invoked function
 */
/* ### move to documentation or rewrite
 * supported three functions:
 * ZVMSetup(char *hint, char answer[])
 * TrapRead(int32_t buffer, int32_t size, int64_t offset)
 * TrapWrite(int32_t buffer, int32_t size, int64_t offset)
 *
 * "args" is an array of syscall name and its arguments:
 * FunctionName(arg1,arg2,..) where arg1/2/3 are values/pointers
 * note: since nacl spoils 1st two arguments if they are pointers, arg[1] are not used
 */
int32_t TrapHandler(struct NaClAppThread *natp, uint32_t args)
{
  uint64_t *sys_args;
  int retcode = 0;

  /* translate address from user space to system. note: cannot set "trap error" */
  sys_args = (uint64_t*)NaClUserToSys(natp->nap, (uintptr_t) args);
  NaClLog(4, "NaClSysNanosleep received in = 0x%lx\n", (intptr_t)sys_args);

  switch(*sys_args)
  {
    case TrapUserSetup:
      retcode = TrapUserSetupHandle(natp->nap, (struct SetupList*) sys_args[2]);
      break;
    case TrapRead:
      retcode = TrapReadHandle(natp->nap,
          (enum ChannelType)sys_args[2], (char*)sys_args[3], (int32_t)sys_args[4], sys_args[5]);
      break;
    case TrapWrite:
      retcode = TrapWriteHandle(natp->nap,
          (enum ChannelType)sys_args[2], (char*)sys_args[3], (int32_t)sys_args[4], sys_args[5]);
      break;
    default:
      retcode = ERR_CODE;
      NaClLog(LOG_ERROR, "function %ld is not supported\n", *sys_args);
      break;
  }

  return retcode;
}

