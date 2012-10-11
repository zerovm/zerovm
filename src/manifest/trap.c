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

/* updates channel tag */
static void UpdateChannelTag(struct ChannelDesc *channel,
    const char *buffer, int32_t size)
{
  int i;

  /* skip if etag is not enabled */
  if(!TagEngineEnabled()) return;

  assert(channel != NULL);
  assert(buffer != NULL);

  /* update etag and log information */
  if(size <= 0) return;
  i = TagUpdate(&channel->tag, buffer, size);
  ErrIf(i == ERR_CODE, "cannot update channel tag");
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

  /* check buffer and convert address */
  if(buffer == NULL) return -EINVAL;
  sys_buffer = (char*)NaClUserToSys(nap, (uintptr_t) buffer);

  /* prevent reading from the closed or not readable channel */
  channel = &nap->system_manifest->channels[ch];
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

  /* check for eof */
  if(channel->eof) return 0;

  /* check limits */
  if(channel->counters[GetsLimit] >= channel->limits[GetsLimit])
    return -EDQUOT;
  if(CHANNEL_RND_READABLE(channel))
    if(offset >= channel->limits[PutSizeLimit] - channel->counters[PutSizeLimit]
      + channel->size) return -EINVAL;

  /* calculate i/o leftovers */
  tail = channel->limits[GetSizeLimit] - channel->counters[GetSizeLimit];
  if(size > tail) size = tail;
  if(size < 1) return -EDQUOT;
  NaClLog(LOG_DEBUG, "channel %s, buffer=0x%lx, size=%d, offset=%ld\n",
      channel->alias, (intptr_t)buffer, size, offset);

  /* read data and update position */
  /* todo(d'b): when the reading operation hits channels end return EOF(-1?) */
  switch(channel->source)
  {
    case LocalFile:
      retcode = pread(channel->handle, sys_buffer, (size_t)size, (off_t)offset);
      if(retcode == -1) retcode = -errno;
      break;
    case NetworkChannel:
      retcode = FetchMessage(channel, sys_buffer, size);
      if(retcode == -1) retcode = -EIO;
      break;
    default: /* design error */
      FailIf(1, "invalid channel source");
      break;
  }

  /* update the channel counter, size, position and tag */
  ++channel->counters[GetsLimit];
  if(retcode > 0)
  {
    channel->counters[GetSizeLimit] += retcode;
    UpdateChannelTag(channel, (const char*)sys_buffer, retcode);

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
    int ch, const char *buffer, int32_t size, int64_t offset)
{
  struct ChannelDesc *channel;
  int64_t tail;
  const char *sys_buffer;
  int32_t retcode = ERR_CODE;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->system_manifest->channels != NULL);

  channel = &nap->system_manifest->channels[ch];

  /* check buffer and convert address */
  if(buffer == NULL) return -EINVAL;
  sys_buffer = (const char*)NaClUserToSys(nap, (uintptr_t) buffer);

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
  tail = channel->limits[PutSizeLimit] - channel->counters[PutSizeLimit];
  if(offset >= channel->limits[PutSizeLimit] &&
      !CHANNEL_READABLE(channel)) return -EINVAL;
  if(offset >= channel->size + tail) return -EINVAL;
  if(size > tail) size = tail;
  if(size < 1) return -EDQUOT;
  NaClLog(LOG_DEBUG, "channel %s, buffer=0x%lx, size=%d, offset=%ld\n",
      channel->alias, (intptr_t)buffer, size, offset);

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

  /* update the channel counter, size, position and tag */
  ++channel->counters[PutsLimit];
  if(retcode > 0)
  {
    channel->counters[PutSizeLimit] += retcode;
    UpdateChannelTag(channel, (const char*)sys_buffer, retcode);

    channel->putpos = offset + retcode;
    channel->size = channel->type == SGetRPut ?
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
  return (int32_t)nap->break_addr;
}

/* accessor: returns the end of the user heap */
static int32_t ZVMHeapEnd(struct NaClApp *nap)
{
  assert(nap != NULL);
  return (int32_t)nap->heap_end;
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

/* this function debug only. return function name by id */
static const char *FunctionNameById(int id)
{
  switch(id)
  {
    case TrapRead: return "TrapRead";
    case TrapWrite: return "TrapWrite";
    case TrapSyscallback: return "TrapSyscallback";
    case TrapChannels: return "TrapChannels";
    case TrapChannelName: return "TrapChannelName";
    case TrapAttributes: return "TrapAttributes";
    case TrapHeapEnd: return "TrapMemSize";
    case TrapHeapPtr: return "TrapHeapPtr";
    case TrapExit: return "TrapExit";
  }
  return "not supported";
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
  NaClLog(LOG_DEBUG, "%s called", FunctionNameById(sys_args[0]));

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
    case TrapHeapEnd:
      retcode = ZVMHeapEnd(nap);
      break;
    case TrapHeapPtr:
      retcode = ZVMHeapPtr(nap);
      break;
    default:
      retcode = -EPERM;
      NaClLog(LOG_ERROR, "function %ld is not supported\n", *sys_args);
      break;
  }

  NaClLog(LOG_DEBUG, "%s returned %d", FunctionNameById(sys_args[0]), retcode);
  return retcode;
}
