/*
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
#include <sys/mman.h>
#include "src/channels/channel.h"
#include "src/main/report.h"
#include "src/main/setup.h"
#include "src/main/config.h"
#include "src/main/zlog.h"
#include "src/syscalls/snapshot.h"
#include "src/syscalls/daemon.h"
#include "src/syscalls/ztrace.h"
#include "src/loader/userspace.h"
#include "src/loader/usermap.h"
#include "src/loader/context.h"

/*
 * read specified amount of bytes from given desc/offset to buffer
 * return amount of read bytes or negative error code if call failed
 */
static int32_t ZVMReadHandle(struct Manifest *manifest,
    int ch, char *buffer, int32_t size, int64_t offset)
{
  struct ChannelDesc *channel;
  int64_t tail;
  char *sys_buffer;

  assert(manifest != NULL);
  assert(manifest->channels != NULL);

  /* check the channel number */
  if(ch < 0 || ch >= manifest->channels->len)
  {
    ZLOGS(LOG_DEBUG, "channel_id=%d, buffer=%p, size=%d, offset=%ld",
        ch, buffer, size, offset);
    return -EINVAL;
  }
  channel = CHANNEL(manifest, ch);
  ZLOGS(LOG_INSANE, "channel %s, buffer=%p, size=%d, offset=%ld",
      channel->alias, buffer, size, offset);

  /* check other arguments sanity */
  if(size < 0) return -EFAULT;
  if(offset < 0) return -EINVAL;
  if(size == 0) return 0;

  /* check buffer availability */
  sys_buffer = (char*)NaClUserToSysAddrNullOkay((uintptr_t)buffer);
  if(CheckUserMap((uintptr_t)sys_buffer, size, PROT_WRITE) == -1)
    return -EINVAL;

  /* ignore user offset for sequential access read */
  if(CH_SEQ_READABLE(channel))
    offset = channel->getpos;
  /* prevent reading beyond the end of the random access channels */
  else
  {
    size = MIN(channel->size - offset, size);
    if(size == 0) return 0;
  }

  /* check for eof */
  if(channel->eof) return 0;

  /* check limits */
  if(channel->counters[GetsLimit] >= channel->limits[GetsLimit])
    return -EDQUOT;

  /* calculate i/o leftovers */
  tail = channel->limits[GetSizeLimit] - channel->counters[GetSizeLimit];
  if(size > tail) size = tail;
  if(size < 1) return -EDQUOT;

  /* read data */
  return ChannelRead(channel, sys_buffer, (size_t)size, (off_t)offset);
}

/*
 * write specified amount of bytes from buffer to given desc/offset
 * return amount of read bytes or negative error code if call failed
 */
static int32_t ZVMWriteHandle(struct Manifest *manifest,
    int ch, const char *buffer, int32_t size, int64_t offset)
{
  struct ChannelDesc *channel;
  int64_t tail;
  const char *sys_buffer;

  assert(manifest != NULL);
  assert(manifest->channels != NULL);

  /* check the channel number */
  if(ch < 0 || ch >= manifest->channels->len)
  {
    ZLOGS(LOG_DEBUG, "channel_id=%d, buffer=%p, size=%d, offset=%ld",
        ch, buffer, size, offset);
    return -EINVAL;
  }
  channel = CHANNEL(manifest, ch);
  ZLOGS(LOG_INSANE, "channel %s, buffer=%p, size=%d, offset=%ld",
      channel->alias, buffer, size, offset);

  /* check other arguments sanity */
  if(size < 0) return -EFAULT;
  if(offset < 0) return -EINVAL;
  if(size == 0) return 0;

  /* check buffer availability */
  sys_buffer = (char*)NaClUserToSysAddrNullOkay((uintptr_t)buffer);
  if(CheckUserMap((uintptr_t)sys_buffer, size, PROT_READ) == -1)
    return -EINVAL;

  /* ignore user offset for sequential access write */
  if(CH_SEQ_WRITEABLE(channel)) offset = channel->putpos;

  /* check limits */
  if(channel->counters[PutsLimit] >= channel->limits[PutsLimit])
    return -EDQUOT;
  tail = channel->limits[PutSizeLimit] - channel->counters[PutSizeLimit];

  /* prevent writing beyond the limit */
  if(CH_RND_WRITEABLE(channel))
    if(offset >= channel->limits[PutSizeLimit])
      return -EINVAL;

  if(offset >= channel->size + tail) return -EINVAL;
  if(size > tail) size = tail;
  if(size < 1) return -EDQUOT;

  /* write data */
  return ChannelWrite(channel, sys_buffer, (size_t)size, (off_t)offset);
}

/*
 * put protection on memory region addr:size. available protections are
 * r/o, r/w, r/x, none. if user asked for r/x validation will be applied
 * returns 0 (successful) or negative error code
 */
static int32_t ZVMProtHandle(uintptr_t addr, uint32_t size, int prot)
{
  int result = 0;
  uintptr_t sysaddr;

  sysaddr = NaClUserToSysAddrNullOkay(addr);

  /* sanity check */
  if(sysaddr % NACL_MAP_PAGESIZE != 0)
    return -EINVAL;

  /* locked regions are not allowed to change protection */
  if(CheckUserMap(sysaddr, size, 0) < 0)
    return -EACCES;

  /* put protection */
  switch(prot)
  {
    case PROT_NONE:
    case PROT_READ:
    case PROT_WRITE:
    case PROT_READ | PROT_WRITE:
      if(Zmprotect((void*)sysaddr, size, prot) != 0)
        result = -errno;
      break;

    case PROT_EXEC:
    case PROT_READ | PROT_EXEC:
      /* test if memory is readable */
      if(CheckUserMap(sysaddr, size, PROT_READ) != 0)
      {
        result = -EACCES;
        break;
      }

      /* validation failed */
      if(Validate((uint8_t*)sysaddr, size, addr) != 0)
      {
        ++ReportSetupPtr()->validation_state;
        result = -EPERM;
      }
      /* validation ok, changing protection */
      else
        if(Zmprotect((void*)sysaddr, size, prot) != 0)
          result = -errno;
      break;

    default:
      result = -EPERM;
      break;
  }

  return result;
}

/* user exit. session is finished. no return. */
static void ZVMExitHandle(struct Manifest *manifest, uint64_t code)
{
  assert(manifest != NULL);

  ReportSetupPtr()->user_code = code;
  if(ReportSetupPtr()->zvm_code == 0)
    ReportSetupPtr()->zvm_state = g_strdup(OK_STATE);
  ZLOGS(LOG_DEBUG, "SESSION %s RETURNED %lu", manifest->node, code);
  SyscallZTrace(TrapEXIT, code);
  SessionDtor(0, OK_STATE);
}

/* handler for syscalls testing */
/* TODO(d'b): to remove before release */
static void ZVMTestHandle(struct Manifest *manifest)
{
  assert(manifest != NULL);

  SaveSession(manifest);
}

int32_t TrapHandler(uint32_t args)
{
  struct Manifest *manifest = GetManifest();
  uint64_t *sargs;
  int retcode = 0;

  /* test args address validity */
  if(args > -48u) return -EFAULT;

  /*
   * translate address from user space to system
   * note: cannot set "trap error"
   */
  sargs = (uint64_t*)NaClUserToSys((uintptr_t)args);
  ZLOGS(LOG_DEBUG, "%s called", FunctionName(*sargs));
  ZTrace("untrusted code");

  switch(*sargs)
  {
#ifndef REMOVE_DEPRECATED
    /* DEPRECATED. API version 1 */
    case TrapFork:
      retcode = Daemon(manifest);
      if(retcode) break;
      SyscallZTrace(TrapFORK, 0);
      ZVMExitHandle(manifest, 0);
      break;
    case TrapExit:
      ZVMExitHandle(manifest, sargs[2]);
      break;
    case TrapRead:
      retcode = ZVMReadHandle(manifest,
          (int)sargs[2], (char*)sargs[3], (int32_t)sargs[4], sargs[5]);
      ZLOGS(LOG_DEBUG, "%s returned %d", FunctionName(TrapREAD), retcode);
      SyscallZTrace(TrapREAD, retcode, sargs[2], sargs[3], sargs[4], sargs[5]);
      return retcode;
      break;
    case TrapWrite:
      retcode = ZVMWriteHandle(manifest,
          (int)sargs[2], (char*)sargs[3], (int32_t)sargs[4], sargs[5]);
      ZLOGS(LOG_DEBUG, "%s returned %d", FunctionName(TrapWRITE), retcode);
      SyscallZTrace(TrapWRITE, retcode, sargs[2], sargs[3], sargs[4], sargs[5]);
      return retcode;
      break;
    case TrapJail:
      retcode = ZVMProtHandle((uint32_t)sargs[2], (uint32_t)sargs[3],
          PROT_READ | PROT_EXEC);
      ZLOGS(LOG_DEBUG, "%s returned %d", FunctionName(TrapPROT), retcode);
      SyscallZTrace(TrapPROT, retcode, sargs[2], sargs[3], PROT_READ | PROT_EXEC);
      return retcode;
      break;
    case TrapUnjail:
      retcode = ZVMProtHandle((uint32_t)sargs[2], (uint32_t)sargs[3],
          PROT_READ | PROT_WRITE);
      ZLOGS(LOG_DEBUG, "%s returned %d", FunctionName(TrapPROT), retcode);
      SyscallZTrace(TrapPROT, retcode, sargs[2], sargs[3], PROT_READ | PROT_EXEC);
      return retcode;
      break;
#endif

    /* API version 2 */
    case TrapFORK:
      retcode = Daemon(manifest);
      if(retcode) break;
      SyscallZTrace(*sargs, 0);
      ZVMExitHandle(manifest, 0);
      break;
    case TrapEXIT:
      ZVMExitHandle(manifest, sargs[1]);
      break;
    case TrapREAD:
      retcode = ZVMReadHandle(manifest,
          (int)sargs[1], (char*)sargs[2], (int32_t)sargs[3], sargs[4]);
      break;
    case TrapWRITE:
      retcode = ZVMWriteHandle(manifest,
          (int)sargs[1], (char*)sargs[2], (int32_t)sargs[3], sargs[4]);
      break;
    case TrapPROT:
      retcode = ZVMProtHandle((uint32_t)sargs[1], (uint32_t)sargs[2],
          (int)sargs[3]);
      break;
    case TrapTest: /* TODO(d'b): to remove before release */
      ZVMTestHandle(manifest);
      ZVMExitHandle(manifest, 0);
      break;

    default:
      retcode = -EPERM;
      ZLOG(LOG_ERROR, "function %ld is not supported", *sargs);
      break;
  }

  /* ztrace and return */
  ZLOGS(LOG_DEBUG, "%s returned %d", FunctionName(*sargs), retcode);
  SyscallZTrace(*sargs, retcode, sargs[1], sargs[2], sargs[3], sargs[4]);
  return retcode;
}
