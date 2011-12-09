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

#include "src/manifest/trap.h"
#include "src/manifest/manifest_setup.h"
#include "src/manifest/mount_channel.h"
#include "src/platform/nacl_log.h"
#include "api/zvm_manifest.h"
#include "src/service_runtime/sel_ldr.h"

/* ### pause cpu time counting */
static void PauseCpuClock(struct SetupList *policy)
{
}

/* ### resume cpu time counting */
static void ResumeCpuClock(struct SetupList *policy)
{
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
  ++nap->manifest->user_setup->cnt_syscalls;
  ++fd->cnt_gets;
  fd->cnt_get_size += size;

  /* read data */
  PauseCpuClock(nap->manifest->user_setup);
  retcode = pread(fd->handle, sys_buffer, (size_t)size, (off_t)offset);
  ResumeCpuClock(nap->manifest->user_setup);

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
  ++nap->manifest->user_setup->cnt_syscalls;
  ++fd->cnt_puts;
  fd->cnt_put_size += size;

  /* read data */
  PauseCpuClock(nap->manifest->user_setup);
  retcode = pwrite(fd->handle, sys_buffer, (size_t)size, (off_t)offset);
  ResumeCpuClock(nap->manifest->user_setup);

  return retcode;
}

///*
// * the only purpouse of this function existance is removing code doubling
// * for TrapReadHandle() and TrapWriteHandle(). (cure worse than the disease?)
// */
//static int32_t TrapAccessHandle(struct NaClApp *nap,
//    int32_t desc, char *buffer, int32_t size, int64_t offset,
//    int32_t *cnt_accs, int32_t *max_accs, int64_t *max_acc_size, int64_t *cnt_acc_size,
//    ssize_t (*func)(int, void*, size_t, off_t), char *func_name)
//{
//  struct PreOpenedFileDesc *fd;
//  int32_t tail;
//  char *sys_buffer;
//  int32_t retcode;
//
//  /* convert address and check buffer */
//  sys_buffer = (char*)NaClUserToSys(nap, (uintptr_t) buffer);
//
//  NaClLog(4, "%s() invoked: desc=%d, buffer=0x%x, size=%d, offset=%d\n",
//      func_name, desc, buffer, size, offset);
//
//  /* take fd from nap with given desc */
//  if(nap == NULL) return -INTERNAL_ERR;
//  fd = GetPreOpenedFileDesc(nap->manifest->user_setup, desc);
//  if(fd == NULL) return -INVALID_DESC;
//
//  /* check arguments sanity */
//  if(size < 1) return -INSANE_SIZE;
//  if(offset < 0) return -INSANE_OFFSET;
//
//  /* check/update limits/counters */
//  if(offset >= fd->fsize) return -OUT_OF_BOUNDS;
//  if(*cnt_accs >= *max_accs) return -OUT_OF_LIMITS;
//
//  tail = *max_acc_size - *cnt_acc_size;
//  if(size > tail) size = tail;
//  if(size < 1) return -OUT_OF_LIMITS;
//
//  /* update counters (even if syscall failed) */
//  ++nap->manifest->user_setup->cnt_syscalls;
//  ++*cnt_accs;
//  *cnt_acc_size += size;
//
//  /* read data */
//  PauseClock();
//  retcode = func(desc, buffer, (size_t)size, (off_t)offset);
//  ResumeClock();
//
//  return retcode;
//}
//
///*
// * read specified amount of bytes from given desc/offset to buffer
// * return amount of read bytes or -1 if call failed
// */
//static int32_t TrapReadHandle(struct NaClApp *nap,
//    int32_t desc, char *buffer, int32_t size, int64_t offset)
//{
//  struct PreOpenedFileDesc *fd;
//  return TrapAccessHandle(desc, buffer, size, offset, fd->cnt_puts, fd->max_puts,
//                          fd->max_put_size, fd->cnt_put_size, pread(), __func__);
//}
//
///*
// * write specified amount of bytes from buffer to given desc/offset
// * return amount of wrote bytes or -1 if call failed
// */
//static int32_t TrapWriteHandle(struct NaClApp *nap,
//    int32_t desc, char *buffer, int32_t size, int64_t offset)
//{
//  struct PreOpenedFileDesc *fd;
//  return TrapAccessHandle(desc, buffer, size, offset, fd->cnt_puts, fd->max_puts,
//                          fd->max_put_size, fd->cnt_put_size, pwrite(), __func__);
//}

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

  /* check/count this call */
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
  STRNCPY_NULL(hint->content_type, policy->content_type, CONTENT_TYPE_LEN);
  STRNCPY_NULL(hint->timestamp, policy->timestamp, TIMESTAMP_LEN);
  STRNCPY_NULL(hint->user_etag, policy->user_etag, USER_TAG_LEN);
  STRNCPY_NULL(hint->x_object_meta_tag, policy->x_object_meta_tag, X_OBJECT_META_TAG_LEN);
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
