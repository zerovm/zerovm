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
#include "src/platform/sel_memory.h"
#include "src/main/setup.h"
#include "src/syscalls/daemon.h"
#include "src/main/ztrace.h"

/*
 * =======================================================================
 * PAYPAL VERSION (with sockets)
 * =======================================================================
 * WARNING: "zerovm sockets" are potential security danger. better way to do
 * "sockets" is support from untrusted side (zrt) and external trusted driver
 * that can be connected via zerovm channel
 */
#ifdef ZVM_SOCKETS
#include <sys/socket.h>
#include <netdb.h>
#include <sys/poll.h>
#endif

/*
 * check "prot" access for user area (start, size)
 * if failed return -1, otherwise - 0
 */
static int CheckRAMAccess(struct NaClApp *nap, uintptr_t start, int64_t size, int prot)
{
  int i;

  start = NaClUserToSysAddrNullOkay(nap, start);
  for(i = LeftBumperIdx; i < MemMapSize; ++i)
  {
    /* skip until start hit block in mem_map */
    if(start >= nap->mem_map[i].end) continue;

    /* fail if block protection doesn't meat prot */
    if((prot & nap->mem_map[i].prot) == 0) return -1;

    /* subtract checked space from given area */
    size -= (nap->mem_map[i].end - start);
    if(size <= 0) return 0;

    start = nap->mem_map[i].end;
  }
  return -1;
}

/*
 * read specified amount of bytes from given desc/offset to buffer
 * return amount of read bytes or negative error code if call failed
 */
static int32_t ZVMReadHandle(struct NaClApp *nap,
    int ch, char *buffer, int32_t size, int64_t offset)
{
  struct ChannelDesc *channel;
  int64_t tail;
  char *sys_buffer;

  assert(nap != NULL);
  assert(nap->manifest != NULL);
  assert(nap->manifest->channels != NULL);

  /* check the channel number */
  if(ch < 0 || ch >= nap->manifest->channels->len)
  {
    ZLOGS(LOG_DEBUG, "channel_id=%d, buffer=%p, size=%d, offset=%ld",
        ch, buffer, size, offset);
    return -EINVAL;
  }
  channel = CH_CH(nap->manifest, ch);
  ZLOGS(LOG_INSANE, "channel %s, buffer=%p, size=%d, offset=%ld",
      channel->alias, buffer, size, offset);

  /* check buffer and convert address */
  if(CheckRAMAccess(nap, (uintptr_t)buffer, size, PROT_WRITE) == -1) return -EINVAL;
  sys_buffer = (char*)NaClUserToSys(nap, (uintptr_t)buffer);

  /* ignore user offset for sequential access read */
  if(CH_SEQ_READABLE(channel))
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
  if(CH_RND_READABLE(channel))
    if(offset >= channel->limits[PutSizeLimit] - channel->counters[PutSizeLimit]
      + channel->size) return -EINVAL;

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
static int32_t ZVMWriteHandle(struct NaClApp *nap,
    int ch, const char *buffer, int32_t size, int64_t offset)
{
  struct ChannelDesc *channel;
  int64_t tail;
  const char *sys_buffer;

  assert(nap != NULL);
  assert(nap->manifest != NULL);
  assert(nap->manifest->channels != NULL);

  /* check the channel number */
  if(ch < 0 || ch >= nap->manifest->channels->len)
  {
    ZLOGS(LOG_DEBUG, "channel_id=%d, buffer=%p, size=%d, offset=%ld",
        ch, buffer, size, offset);
    return -EINVAL;
  }
  channel = CH_CH(nap->manifest, ch);
  ZLOGS(LOG_INSANE, "channel %s, buffer=%p, size=%d, offset=%ld",
      channel->alias, buffer, size, offset);

  /* check buffer and convert address */
  if(CheckRAMAccess(nap, (uintptr_t)buffer, size, PROT_READ) == -1) return -EINVAL;
  sys_buffer = (char*)NaClUserToSys(nap, (uintptr_t) buffer);

  /* ignore user offset for sequential access write */
  if(CH_SEQ_WRITEABLE(channel)) offset = channel->putpos;

  /* check arguments sanity */
  if(size == 0) return 0; /* success. user has read 0 bytes */
  if(size < 0) return -EFAULT;
  if(offset < 0) return -EINVAL;

  /* check limits */
  if(channel->counters[PutsLimit] >= channel->limits[PutsLimit])
    return -EDQUOT;
  tail = channel->limits[PutSizeLimit] - channel->counters[PutSizeLimit];
  if(offset >= channel->limits[PutSizeLimit] &&
      !((CH_RW_TYPE(channel) & 1) == 1)) return -EINVAL;

  if(offset >= channel->size + tail) return -EINVAL;
  if(size > tail) size = tail;
  if(size < 1) return -EDQUOT;

  /* write data */
  return ChannelWrite(channel, sys_buffer, (size_t)size, (off_t)offset);
}

#define JAIL_CHECK \
    uintptr_t sysaddr; \
    int result; \
\
    assert(nap != NULL); \
    assert(nap->manifest != NULL); \
\
    sysaddr = NaClUserToSysAddrNullOkay(nap, addr); \
\
    /* sanity check */ \
    if(size <= 0) return -EINVAL; \
    if(sysaddr < nap->mem_map[HeapIdx].start || \
        sysaddr >= nap->mem_map[HeapIdx].end) return -EINVAL; \
    if(sysaddr != ROUNDDOWN_64K(sysaddr)) return -EINVAL

/*
 * validate given buffer and, if successful, change protection to
 * read / execute and return 0
 */
static int32_t ZVMJailHandle(struct NaClApp *nap, uintptr_t addr, int32_t size)
{
  JAIL_CHECK;

  /* validate */
  result = NaClSegmentValidates((uint8_t*)sysaddr, size, sysaddr);
  if(result == 0) return -EPERM;

  /* protect */
  result = NaCl_mprotect((void*)sysaddr, size, PROT_READ | PROT_EXEC);
  if(result != 0) return -EACCES;

  return 0;
}

/* change protection to read / write and return 0 if successful */
static int32_t ZVMUnjailHandle(struct NaClApp *nap, uintptr_t addr, int32_t size)
{
  JAIL_CHECK;

  /* protect */
  result = NaCl_mprotect((void*)sysaddr, size, PROT_READ | PROT_WRITE);
  if(result != 0) return -EACCES;

  return 0;
}
#undef JAIL_CHECK

/* user exit. session is finished */
static void ZVMExitHandle(struct NaClApp *nap, int32_t code)
{
  assert(nap != NULL);

  SetUserCode(code);
  if(GetExitCode() == 0)
    SetExitState(OK_STATE);
  ZLOGS(LOG_DEBUG, "SESSION %d RETURNED %d", nap->manifest->node, code);
  ReportDtor(0);
}

/*
 * note: variadic macros does not comply to c89 standard, but since this
 *   code is only demonstrates possibility to work with sockets it is ok
 */
#ifdef ZVM_SOCKETS

/* utility macros */
#define EAT(x)
#define REM(x) x
#define STRIP(x) EAT x
#define PAIR(x) REM x

/* This counts the number of args */
#define NARGS_SEQ(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define NARGS(...) NARGS_SEQ(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1)

/* This will let macros expand before concating them */
#define PRIMITIVE_CAT(x, y) x ## y
#define CAT(x, y) PRIMITIVE_CAT(x, y)

/* This will call a macro on each argument passed in */
#define APPLY(macro, ...) CAT(APPLY_, NARGS(__VA_ARGS__))(macro, __VA_ARGS__)
#define APPLY_1(m, x1) m(x1)
#define APPLY_2(m, x1, x2) m(x1), m(x2)
#define APPLY_3(m, x1, x2, x3) m(x1), m(x2), m(x3)
#define APPLY_4(m, x1, x2, x3, x4) m(x1), m(x2), m(x3), m(x4)
#define APPLY_5(m, x1, x2, x3, x4, x5) m(x1), m(x2), m(x3), m(x4), m(x5)
#define APPLY_6(m, x1, x2, x3, x4, x5, x6) m(x1), m(x2), m(x3), m(x4), m(x5), m(x6)
#define APPLY_7(m, x1, x2, x3, x4, x5, x6, x7) m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7)
#define APPLY_8(m, x1, x2, x3, x4, x5, x6, x7, x8) m(x1), m(x2), m(x3), m(x4), m(x5), m(x6), m(x7), m(x8)

/* function template #1 */
#define FUNC(name, ...) \
  int ZVM_ ## name(APPLY(PAIR, __VA_ARGS__)) \
  { \
    int result = name(APPLY(STRIP, __VA_ARGS__)); \
    return result == -1 ? -errno : result; \
  }

/* function template #2 */
#define FUNC2(name, retype, ...) \
  retype ZVM_ ## name(APPLY(PAIR, __VA_ARGS__)) \
  { \
    retype result = name(APPLY(STRIP, __VA_ARGS__)); \
    return result == NULL ? (void*)(intptr_t)-h_errno : result; \
  }

FUNC(socket, (int)domain, (int)type, (int)protocol)
FUNC(bind, (int)sockfd, (const struct sockaddr*)addr, (socklen_t)addrlen)
FUNC(connect, (int)sockfd, (const struct sockaddr*)addr, (socklen_t)addrlen)
FUNC(accept, (int)sockfd, (struct sockaddr*)addr, (socklen_t*)addrlen)
FUNC(listen, (int)sockfd, (int)backlog)
FUNC(recv, (int)sockfd, (void*)buf, (size_t)len, (int)flags)
FUNC(recvfrom, (int)sockfd, (void*)buf, (size_t)len, (int)flags, (struct sockaddr*)src_addr, (socklen_t*)addrlen)
FUNC(recvmsg, (int)sockfd, (struct msghdr*)msg, (int)flags)
FUNC(send, (int)sockfd, (const void*)buf, (size_t)len, (int)flags)
FUNC(sendto, (int)sockfd, (const void*)buf, (size_t)len, (int)flags, (const struct sockaddr*)dest_addr, (socklen_t)addrlen)
FUNC(sendmsg, (int)sockfd, (const struct msghdr*)msg, (int)flags)
FUNC(getsockopt, (int)sockfd, (int)level, (int)optname, (void*)optval, (socklen_t*)optlen)
FUNC(setsockopt, (int)sockfd, (int)level, (int)optname, (const void*)optval, (socklen_t)optlen)
FUNC(select, (int)nfds, (fd_set*)readfds, (fd_set*)writefds, (fd_set*)exceptfds, (struct timeval*)timeout)
FUNC(poll, (struct pollfd*)fds, (nfds_t)nfds, (int)timeout)
FUNC2(gethostbyname, struct hostent*, (const char*)name)
FUNC2(gethostbyaddr, struct hostent*, (const void*)addr, (socklen_t)len, (int)type)
FUNC(close, (int)fd)
#endif

int32_t TrapHandler(struct NaClApp *nap, uint32_t args)
{
  uint64_t *sargs;
  int retcode = 0;

  assert(nap != NULL);
  assert(nap->manifest != NULL);

  /*
   * translate address from user space to system
   * note: cannot set "trap error"
   */
  sargs = (uint64_t*)NaClUserToSys(nap, (uintptr_t)args);
  ZLOGS(LOG_DEBUG, "%s called", FunctionName(*sargs));
  ZTrace("untrusted code");

  switch(*sargs)
  {
    case TrapFork:
      retcode = Daemon(nap);
      if(retcode) break;
      SyscallZTrace(*sargs, 0);
      SyscallZTrace(TrapExit, 0);
      ZVMExitHandle(nap, 0);
      break;
    case TrapExit:
      SyscallZTrace(*sargs, sargs[2]);
      ZVMExitHandle(nap, (int32_t)sargs[2]);
      break;
    case TrapRead:
      retcode = ZVMReadHandle(nap,
          (int)sargs[2], (char*)sargs[3], (int32_t)sargs[4], sargs[5]);
      break;
    case TrapWrite:
      retcode = ZVMWriteHandle(nap,
          (int)sargs[2], (char*)sargs[3], (int32_t)sargs[4], sargs[5]);
      break;
    case TrapJail:
      retcode = ZVMJailHandle(nap, (uint32_t)sargs[2], (int32_t)sargs[3]);
      break;
    case TrapUnjail:
      retcode = ZVMUnjailHandle(nap, (uint32_t)sargs[2], (int32_t)sargs[3]);
      break;

#ifdef ZVM_SOCKETS
    case TrapSocket:
      retcode = ZVM_socket((int)sargs[2], (int)sargs[3], (int)sargs[4]);
      break;
    case TrapBind: {
      const struct sockaddr *addr = (void*)NaClUserToSys(nap, (uintptr_t)sargs[3]);
      retcode = ZVM_bind((int)sargs[2], addr, (socklen_t)sargs[4]);
      break; }
    case TrapConnect: {
      const struct sockaddr *addr = (void*)NaClUserToSys(nap, (uintptr_t)sargs[3]);
      retcode = ZVM_connect((int)sargs[2], addr, (socklen_t)sargs[4]);
      break; }
    case TrapAccept: {
      struct sockaddr *addr = (void*)NaClUserToSys(nap, (uintptr_t)sargs[3]);
      socklen_t *len = (void*)NaClUserToSys(nap, (uintptr_t)sargs[4]);
      retcode = ZVM_accept((int)sargs[2], addr, len);
      break; }
    case TrapListen:
      retcode = ZVM_listen((int)sargs[2], (int)sargs[3]);
      break;
    case TrapRecv: {
      void *buf = (void*)NaClUserToSys(nap, (uintptr_t)sargs[3]);
      retcode = ZVM_recv((int)sargs[2], buf, (size_t)sargs[4], (int)sargs[5]);
      break; }
    case TrapRecvfrom: {
      void *buf = (void*)NaClUserToSys(nap, (uintptr_t)sargs[3]);
      struct sockaddr *addr = (void*)NaClUserToSys(nap, (uintptr_t)sargs[6]);
      socklen_t *len = (void*)NaClUserToSys(nap, (uintptr_t)sargs[7]);
      retcode = ZVM_recvfrom((int)sargs[2], buf, (size_t)sargs[4], (int)sargs[5], addr, len);
      break; }
    case TrapRecvmsg: {
      struct msghdr *msg = (void*)NaClUserToSys(nap, (uintptr_t)sargs[3]);
      retcode = ZVM_recvmsg((int)sargs[2], msg, (int)sargs[4]);
      break; }
    case TrapSend: {
      const void *buf = (void*)NaClUserToSys(nap, (uintptr_t)sargs[3]);
      retcode = ZVM_send((int)sargs[2], buf, (size_t)sargs[4], (int)sargs[5]);
      break; }
    case TrapSendto: {
      const void *buf = (void*)NaClUserToSys(nap, (uintptr_t)sargs[3]);
      const struct sockaddr *addr = (void*)NaClUserToSys(nap, (uintptr_t)sargs[6]);
      retcode = ZVM_sendto((int)sargs[2], buf, (size_t)sargs[4], (int)sargs[5], addr, (socklen_t)sargs[7]);
      break; }
    case TrapSendmsg: {
      const struct msghdr *msg = (void*)NaClUserToSys(nap, (uintptr_t)sargs[3]);
      retcode = ZVM_sendmsg((int)sargs[2], msg, (int)sargs[4]);
      break; }
    case TrapGetsockopt: {
      void *optval = (void*)NaClUserToSys(nap, (uintptr_t)sargs[5]);
      socklen_t *len = (void*)NaClUserToSys(nap, (uintptr_t)sargs[6]);
      retcode = ZVM_getsockopt((int)sargs[2], (int)sargs[3], (int)sargs[4], optval, len);
      break; }
    case TrapSetsockopt: {
      const void *optval = (void*)NaClUserToSys(nap, (uintptr_t)sargs[5]);
      retcode = ZVM_setsockopt((int)sargs[2], (int)sargs[3], (int)sargs[4], optval, (socklen_t)sargs[6]);
      break; }
    case TrapSelect: {
      fd_set *readfds = (void*)NaClUserToSys(nap, (uintptr_t)sargs[3]);
      fd_set *writefds = (void*)NaClUserToSys(nap, (uintptr_t)sargs[4]);
      fd_set *exceptfds = (void*)NaClUserToSys(nap, (uintptr_t)sargs[5]);
      struct timeval *timeout = (void*)NaClUserToSys(nap, (uintptr_t)sargs[6]);
      retcode = ZVM_select((int)sargs[2], readfds, writefds, exceptfds, timeout);
      break; }
    case TrapPoll: {
      struct pollfd *fds = (void*)NaClUserToSys(nap, (uintptr_t)sargs[2]);
      retcode = ZVM_poll(fds, (nfds_t)sargs[3], (int)sargs[4]);
      break; }
    case TrapGethostbyname: {
      const char *name = (void*)NaClUserToSys(nap, (uintptr_t)sargs[2]);
      retcode = (int)(intptr_t)ZVM_gethostbyname(name);
      break; }
    case TrapGethostbyaddr: {
      const void *addr = (void*)NaClUserToSys(nap, (uintptr_t)sargs[2]);
      retcode = (int)(intptr_t)ZVM_gethostbyaddr(addr, (socklen_t)sargs[3], (int)sargs[4]);
      break; }
    case TrapClose:
      retcode = ZVM_close((int)sargs[2]);
      break;
#endif

    default:
      retcode = -EPERM;
      ZLOG(LOG_ERROR, "function %ld is not supported", *sargs);
      break;
  }

  /* report, ztrace and return */
  FastReport();
  ZLOGS(LOG_DEBUG, "%s returned %d", FunctionName(*sargs), retcode);
  SyscallZTrace(*sargs, retcode, sargs[2], sargs[3], sargs[4], sargs[5], sargs[6], sargs[7]);
  return retcode;
}
