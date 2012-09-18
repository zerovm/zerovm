/*
 * "single syscall" engine
 * this is the replacement for nacl multiple syscalls. however we still use
 * trampoline engine to translate invocations to system from user space
 *
 *  Created on: Dec 2, 2011
 *      Author: d'b
 */

#include <assert.h>
#include "src/service_runtime/etag.h"
#include "src/manifest/trap.h"
#include "src/manifest/manifest_setup.h"
#include "src/manifest/prefetch.h"
#include "src/service_runtime/nacl_globals.h"

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

/* user exit. invokes long jump to main(). uses global var */
static int32_t ZVMExitHandle(struct NaClApp *nap, int32_t code)
{
  assert(nap != NULL);
  nap->system_manifest->user_ret_code = code;
  NaClLog(1, "Exit syscall handler: %d\n", code);
  longjmp(user_exit, code);

  return code; /* prevent compiler warning. not reached */
}

/*
 * returns mask of the channel accessibility considering if the channel
 * exceeded limits and cannot be read (and/or) write
 * 0x0 - inaccessible channel
 * 0x1 - read only
 * 0x2 - write only
 * 0x3 - read/write
 * note: for the cdr 0x1 means the channel exceeded write limit
 */
int ChannelIOMask(struct ChannelDesc *channel)
{
  uint32_t rw = 0;
  int64_t puts_rest = channel->limits[PutsLimit] - channel->counters[PutsLimit];
  int64_t putsize_rest = channel->limits[PutSizeLimit] - channel->counters[PutSizeLimit];
  int64_t gets_rest = channel->limits[GetsLimit] - channel->counters[GetsLimit];
  int64_t getsize_rest = channel->limits[GetSizeLimit] - channel->counters[GetSizeLimit];

  rw |= gets_rest && getsize_rest;
  rw |= (puts_rest && putsize_rest) << 1;

  return rw;
}

/* updates channel tag and log it with i/o results */
static void UpdateChannelTag(struct ChannelDesc *channel,
    char *buffer, int32_t size, int64_t offset)
{
  char *hex;

  /* skip if etag is not enabled */
  if(!EtagEnabled()) return;

  assert(channel != NULL);
  assert(buffer != NULL);

  /* update etag and log information */
  hex = (char*)UpdateEtag(&channel->tag, buffer, size);
  NaClLog(LOG_DEBUG, "buf = %lX, size = %d, offset = %ld, etag = %s",
      (uintptr_t)buffer, size, offset, hex);
}

/*
 * read specified amount of bytes from given desc/offset to buffer
 * return amount of read bytes or negative error code if call failed
 */
int32_t ZVMReadHandle(struct NaClApp *nap,
    int ch, char *buffer, int32_t size, int64_t offset)
{
  struct ChannelDesc *channel;
  int64_t tail;
  char *sys_buffer;
  int32_t retcode = ERR_CODE;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->system_manifest->channels != NULL);
  assert(nap->system_manifest->channels_count > 0);

  /* todo(d'b): check buffer address and size to prevent SYGSEGV */

  /* check buffer and convert address */
  if(buffer == NULL) return -EINVAL;
  sys_buffer = (char*)NaClUserToSys(nap, (uintptr_t) buffer);
  NaClLog(LOG_DEBUG, "%s() invoked: desc=%d, buffer=0x%lx, size=%d, offset=%ld\n",
      __func__, ch, (intptr_t)buffer, size, offset);

  channel = &nap->system_manifest->channels[ch];

  /* prevent reading from the closed or not readable channel */
  if(channel->closed) return -EBADF;
  if(!CHANNEL_READABLE(channel)) return -EBADF;

  /* ignore user offset for sequential access read */
  if(CHANNEL_SEQ_READABLE(channel))
    offset = channel->getpos;
  else
    /* prevent reading beyond the end of the random access channels */
    size = MIN(channel->size - offset, size);

  /* check arguments sanity */
  if(size == 0) return 0; /* success. user has read 0 bytes */
  if(size < 0) return -EFAULT;
  if(offset < 0) return -EINVAL;

  /* check for NET_EOF */
  if(channel->source == NetworkChannel
      && channel->bufpos == NET_EOF) return 0;

  /* check limits */
  if(channel->counters[GetsLimit] >= channel->limits[GetsLimit])
    return -EDQUOT;
  if(offset >= channel->limits[GetSizeLimit])
    return -EINVAL;

  /* calculate i/o leftovers */
  tail = channel->limits[GetSizeLimit] - channel->counters[GetSizeLimit];
  if(size > tail) size = tail;
  if(size < 1) return -EDQUOT;

  /* update counters */
  ++channel->counters[GetsLimit];
  channel->counters[GetSizeLimit] += size;

  /* read data and update position */
  /* todo(d'b): when the reading operation hits channels end return EOF(-1?) */
  switch(channel->source)
  {
    case LocalFile:
      retcode = pread(channel->handle, sys_buffer, (size_t)size, (off_t)offset);
      if(retcode == -1) retcode = -errno;
      else if(retcode == 0) retcode = ZVM_EOF;
      break;
    case NetworkChannel:
      retcode = FetchMessage(channel, sys_buffer, size);
      if(retcode == -1) retcode = -EIO;
      break;
    default: /* design error */
      FailIf(1, "invalid channel source");
      break;
  }

  /* update the channel position and tag */
  if(retcode > 0)
  {
    UpdateChannelTag(channel, sys_buffer, retcode, offset);

    /*
     * current get cursor. must be updated if channel have seq get
     * but there is nothing wrong to update it even it have random get
     */
    channel->getpos = offset + retcode;

    /* if channel have random put update put cursor. not allowed for cdr */
    if(CHANNEL_RND_WRITEABLE(channel)) channel->putpos = offset + retcode;
  }

  return retcode;
}

/*
 * write specified amount of bytes from buffer to given desc/offset
 * return amount of read bytes or negative error code if call failed
 */
int32_t ZVMWriteHandle(struct NaClApp *nap,
    int ch, char *buffer, int32_t size, int64_t offset)
{
  struct ChannelDesc *channel;
  int64_t tail;
  char *sys_buffer;
  int32_t retcode = ERR_CODE;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert((void*)nap->system_manifest->channels != NULL);

  /* todo(d'b): check buffer address and size to prevent SYGSEGV */

  /* check buffer and convert address */
  if(buffer == NULL) return -EINVAL;
  sys_buffer = (char*)NaClUserToSys(nap, (uintptr_t) buffer);
  NaClLog(LOG_DEBUG, "%s() invoked: desc=%d, buffer=0x%lx, size=%d, offset=%ld\n",
        __func__, ch, (intptr_t)buffer, size, offset);

  channel = &nap->system_manifest->channels[ch];

  /* prevent writing to the closed channel */
  if(channel->closed) return -EBADF;

  /*
   * special case: the user called to close the channel
   * note: the channel can have any type of access even read-only
   */
  if(size == ZVM_EOF && offset == ZVM_EOF)
  {
    if(channel->closed) return -1;
    channel->closed = 1;

    /* for a w/o network channel EOF should be send */
    /* todo(d'b): .. */
    return 0;
  }

  /* prevent writing to the not writable channel */
  if(CHANNEL_WRITEABLE(channel) == 0) return -EBADF;

  /* ignore user offset for sequential access write */
  if(CHANNEL_SEQ_WRITEABLE(channel)) offset = channel->putpos;

  /* check arguments sanity */
  if(size == 0) return 0; /* success. user has read 0 bytes */
  if(size < 0) return -EFAULT;
  if(offset < 0) return -EINVAL;

  /* check limits */
  if(channel->counters[PutsLimit] >= channel->limits[PutsLimit])
    return -EDQUOT;
  if(offset >= channel->limits[PutSizeLimit])
    return -EINVAL;
  tail = channel->limits[PutSizeLimit] - channel->counters[PutSizeLimit];
  if(size > tail) size = tail;
  if(size < 1) return -EDQUOT;

  /* update counters */
  ++channel->counters[PutsLimit];
  channel->counters[PutSizeLimit] += size;

  /* write data and update position */
  switch(channel->source)
  {
    case LocalFile:
      retcode = pwrite(channel->handle, sys_buffer, (size_t)size, (off_t)offset);
      if(retcode == -1) retcode = -errno;
      break;
    case NetworkChannel:
      retcode = SendMessage(channel, sys_buffer, size);
      if(retcode == -1) retcode = -EIO;
      break;
    default: /* design error */
      FailIf(1, "invalid channel source");
      break;
  }

  /* update the channel position and tag */
  if(retcode > 0)
  {
    UpdateChannelTag(channel, sys_buffer, retcode, offset);

    channel->putpos = offset + retcode;
    channel->size = channel->type == 2 ?
        MAX(channel->size, channel->putpos) : channel->putpos;
    channel->getpos = channel->putpos;
  }

  return retcode;
}

/*
 * ZeroVM API. accessors and initializers
 */

/* accessor: returns user heap start address */
static int32_t ZVMHeapPtr(struct NaClApp *nap)
{
  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  return nap->system_manifest->heap_ptr;
}

/* accessor: returns memory size available for user program, 0 means 4gb */
static int32_t ZVMMemSize(struct NaClApp *nap)
{
  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  return nap->system_manifest->max_mem;
}

/* accessor: how much syscalls allowed for the user */
static int32_t ZVMSyscallsLimit(struct NaClApp *nap)
{
  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  return nap->system_manifest->max_syscalls;
}

/* accessor: how much syscalls the user already used */
static int32_t ZVMSyscallsCount(struct NaClApp *nap)
{
  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  return nap->system_manifest->cnt_syscalls;
}

/*
 * return channel name length + 1 if given pointer is not NULL
 * and "name" field in the given channel descriptor is NULL
 * otherwise also copy channel alias to "name" field
 */
int32_t ZVMChannelName(struct NaClApp *nap, struct ZVMChannel *chnl, int ch)
{
  struct ZVMChannel *uchannel;
  struct ChannelDesc *channel;
  char *alias;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  if(chnl == NULL) return -EINVAL;

  uchannel = (struct ZVMChannel*)NaClUserToSys(nap, (uintptr_t)chnl);
  channel = &nap->system_manifest->channels[ch];

  /* user asked for the name. give her the channel alias */
  if(uchannel->name != NULL)
  {
    alias = (char*)NaClUserToSys(nap, (uintptr_t)uchannel->name);
    strcpy(alias, channel->alias);
  }

  return strlen(channel->alias) + 1;
}

/*
 * initializer: channels
 * returns channels number if buffer == NULL, otherwise
 * returns channels number and initializes given buffer with channels data
 */
static int32_t ZVMChannels(struct NaClApp *nap, struct ZVMChannel *buf)
{
  struct ZVMChannel *uchannels;
  struct ChannelDesc *channels;
  int ch;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /* user asked for the channels count */
  if(buf == NULL) return nap->system_manifest->channels_count;

  channels = nap->system_manifest->channels;
  uchannels = (struct ZVMChannel*)NaClUserToSys(nap, (uintptr_t)buf);

  /* populate given array with the channels information */
  for(ch = 0; ch < nap->system_manifest->channels_count; ++ch)
  {
    int i;

    uchannels[ch].name = NULL; /* see ZVMChannelName() */
    uchannels[ch].type = channels[ch].type;

    /* copy limits and counters */
    for(i = 0; i < IOLimitsCount; ++i)
      uchannels[ch].limits[i] = channels[ch].limits[i];

    /* channel size/position */
    switch(uchannels[ch].type)
    {
      case SGetSPut:
        /* size/position is not defined */
        break;
      case SGetRPut:
        uchannels[ch].size = channels[ch].size;
        break;
      case RGetSPut:
        uchannels[ch].size = channels[ch].size;
        break;
      case RGetRPut:
        /* in this case get or put updates both positions synchronously */
        uchannels[ch].size = channels[ch].size;
        break;
      default:
        /* invalid access type */
        break;
    }
  }

  return nap->system_manifest->channels_count;
}

/*
 * set syscallback
 * returns a new syscallback when successfully installed or
 * returns an old syscallback if installation failed
 * note: global syscallback uses system address space,
 *       nap->system_manifest->syscallback - user space
 */
static int32_t ZVMSyscallback(struct NaClApp *nap, int32_t addr)
{
  int32_t good_pos = 0;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /* uninstall syscallback if 0 given */
  if(addr == 0)
  {
    syscallback = 0; /* global variable */
    nap->system_manifest->syscallback = 0;
    return 0;
  }

  /* check alignement */
  if(addr & (OP_ALIGNEMENT - 1))
    return nap->system_manifest->syscallback;

  /* check if syscallback points to text */
  good_pos |= addr >= NACL_TRAMPOLINE_END && addr < nap->static_text_end;
  good_pos |= addr >= nap->dynamic_text_start && addr < nap->dynamic_text_end;

  if(good_pos)
  {
    nap->system_manifest->syscallback = addr;
    syscallback = NaClUserToSys(nap, (intptr_t) addr);
  }

  return nap->system_manifest->syscallback;
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
  NaClLog(LOG_SUICIDE, "Trap arguments address = 0x%lx\n", (intptr_t)sys_args);

  switch(*sys_args)
  {
    case TrapExit:
      retcode = ZVMExitHandle(nap, (int32_t) sys_args[2]);
      break;
    case TrapRead:
      retcode = ZVMReadHandle(nap,
          (int)sys_args[2], (char*)sys_args[3], (int32_t)sys_args[4], sys_args[5]);
      break;
    case TrapWrite:
      retcode = ZVMWriteHandle(nap,
          (int)sys_args[2], (char*)sys_args[3], (int32_t)sys_args[4], sys_args[5]);
      break;
    case TrapSyscallback:
      retcode = ZVMSyscallback(nap, (int32_t)sys_args[2]);
      break;
    case TrapChannels:
      retcode = ZVMChannels(nap, (struct ZVMChannel*)sys_args[2]);
      break;
    case TrapChannelName:
      retcode = ZVMChannelName(nap, (struct ZVMChannel*)sys_args[2], (int32_t)sys_args[3]);
      break;
    case TrapSyscallsCount:
      retcode = ZVMSyscallsCount(nap);
      break;
    case TrapSyscallsLimit:
      retcode = ZVMSyscallsLimit(nap);
      break;
    case TrapMemSize:
      retcode = ZVMMemSize(nap);
      break;
    case TrapHeapPtr:
      retcode = ZVMHeapPtr(nap);
      break;

    default:
      retcode = -EPERM;
      NaClLog(LOG_ERROR, "function %ld is not supported\n", *sys_args);
      break;
  }

  NaClLog(LOG_SUICIDE, "leaving Trap with code = 0x%x\n", retcode);
  return retcode;
}
