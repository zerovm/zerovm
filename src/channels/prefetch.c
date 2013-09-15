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
#include <arpa/inet.h> /* convert ip <-> int */
#include <zmq.h>
#include "src/channels/prefetch.h"
#include "src/main/report.h"

#define LINGER 50
#define LOWEST_AVAILABLE_PORT 49152
#define NET_BUFFER_SIZE BUFFER_SIZE

/* todo(d'b): find more neat solution than put it twice */
#define XARRAY(a) static char *ARRAY_##a[] = {a};
#define X(a) #a,
  XARRAY(PROTOCOLS)
#undef X

/* fail session if return code is not 0 */
#define ZMQ_FAIL(code) \
    ZLOGFAIL(code, EIO, "failed: %s", zmq_strerror(zmq_errno()))

static void *context = NULL; /* zeromq context */

/* make url from (Connection*) and return it through the "url" parameter */
static void MakeURL(struct ChannelDesc *channel, int n, char *url, int32_t size)
{
  char host[BIG_ENOUGH_STRING] = "*";

  /* create string with host */
  if(IS_WO(channel) || IS_IPHOST(CH_FLAGS(channel, n)))
  {
    struct in_addr ip;
    ip.s_addr = CH_HOST(channel, n);
    g_snprintf(host, BIG_ENOUGH_STRING, "%s", inet_ntoa(ip));
  }

  /* construct url */
  g_snprintf(url, size, "%s://%s:%u",
      g_ascii_strdown(XSTR(PROTOCOLS, CH_PROTO(channel, n)), -1),
      host, CH_PORT(channel, n));

  ZLOG(LOG_INSANE, "url = %s", url);
}

/* bind the RO source */
static void Bind(struct ChannelDesc *channel, int n)
{
  struct Connection *c = CH_SOURCE(channel, n);
  static uint16_t port = LOWEST_AVAILABLE_PORT;
  char url[BIG_ENOUGH_STRING];
  int result = -1;

  /* loop will end anyway after "port" overflow */
  for(;port >= LOWEST_AVAILABLE_PORT; ++port)
  {
    if(!IS_IPHOST(c->flags)) c->port = port;
    MakeURL(channel, n, url, BIG_ENOUGH_STRING);
    result = zmq_bind(CH_HANDLE(channel, n), url);
    if(result == 0 || IS_IPHOST(c->flags)) break;
  }

  ZLOG(LOG_DEBUG, "host = %u, port = %u", c->host, c->port);
  ZLOGFAIL(result != 0, EFAULT, "cannot bind %s %d: %s", channel->alias, n,
      strerror(errno));
}

/* connect the WO source */
static void Connect(struct ChannelDesc *channel, int n)
{
  char url[BIG_ENOUGH_STRING];
  uint64_t hwm = 1;
  int linger = LINGER;
  void *h = CH_HANDLE(channel, n);

  /* disable linger and block on send */
  ZMQ_FAIL(zmq_setsockopt(h, ZMQ_LINGER, &linger, sizeof linger));
  ZMQ_FAIL(zmq_setsockopt(h, ZMQ_HWM, &hwm, sizeof hwm));
  MakeURL(channel, n, url, BIG_ENOUGH_STRING);
  ZLOGS(LOG_DEBUG, "connect url %s to %s %d", url, channel->alias, n);
  ZMQ_FAIL(zmq_connect(h, url));
}

void NetCtor(const struct Manifest *manifest)
{
  /* get zmq context */
  context = zmq_init(1);
  ZLOGFAIL(context == NULL, EFAULT, "cannot initialize zeromq context");
}

void NetDtor(struct Manifest *manifest)
{
  /* don't terminate if session is broken */
  if(GetExitCode() != 0) return;

  zmq_term(context);
  context = NULL;
}

char *MessageData(struct ChannelDesc *channel)
{
  return (char*)zmq_msg_data(channel->msg);
}

/* get the next message. updates channel->msg (and indices) */
static void GetMessage(struct ChannelDesc *channel, int n)
{
  assert(channel->eof == 0);
  ZLOG(LOG_DEBUG, "entered %s:%d", channel->alias, n);

  /* re-initialize message */
  ZMQ_FAIL(zmq_msg_close(channel->msg));
  ZMQ_FAIL(zmq_msg_init(channel->msg));

  /* receive the next message and rewind buffer */
  ZMQ_FAIL(zmq_recv(CH_HANDLE(channel, n), channel->msg, 0));
  channel->bufend = zmq_msg_size(channel->msg);
  channel->bufpos = 0;
}

void FetchMessage(struct ChannelDesc *channel, int n)
{
  ZLOG(LOG_DEBUG, "entered %s:%d", channel->alias, n);

  /* get message */
  GetMessage(channel, n);

  /* if eof detected. get the last part of eof */
  if(channel->bufend > 0) return;
  GetMessage(channel, n);
  channel->eof = 1;

  /* 2nd part of eof */
  if(channel->bufend == 0) return;
  if(channel->bufend != TAG_DIGEST_SIZE)
  {
    ZLOG(LOG_ERROR, "invalid eof size = %d", channel->bufend);
    channel->bufend = 0;
  }
}

/* send message "channel->msg" */
static void SendMessage(struct ChannelDesc *channel, int n)
{
  ZLOG(LOG_DEBUG, "send to source %s;%d", channel->alias, n);
  ZMQ_FAIL(zmq_send(CH_HANDLE(channel, n), channel->msg, 0));
  ZLOG(LOG_DEBUG, "done");
}

int32_t SendData(struct ChannelDesc *channel, int n, const char *buf, int32_t count)
{
  int32_t writerest;

  assert(channel != NULL);
  assert(buf != NULL);

  /* send a buffer through the multiple messages */
  ZLOG(LOG_DEBUG, "channel %s, buffer=0x%lx, size=%d",
      channel->alias, (intptr_t)buf, count);
  for(writerest = count; writerest > 0; writerest -= NET_BUFFER_SIZE)
  {
    int32_t towrite = MIN(writerest, NET_BUFFER_SIZE);

    /* create the message */
    if(writerest == towrite)
    {
      ZMQ_FAIL(zmq_msg_init_size(channel->msg, towrite));
      memcpy(zmq_msg_data(channel->msg), buf, towrite);
    }
    /* create the message (zero-copy) */
    else
      ZMQ_FAIL(zmq_msg_init_data(channel->msg, (void*)buf, towrite, NULL, NULL));

    /* send and free the message */
    SendMessage(channel, n);
    buf += towrite;
    ZMQ_FAIL(zmq_msg_close(channel->msg));
  }

  return count;
}

void SyncSource(struct ChannelDesc *channel, int n)
{
  if(!CH_SEQ_READABLE(channel)) return;

  ZLOGS(LOG_DEBUG, "%s;%d before skip pos = %ld, getpos = %ld",
      channel->alias, n, CH_SOURCE(channel, n)->pos, channel->getpos);

  /* if source is a pipe read (*->getpos - *->pos) bytes */
  if(CH_PROTO(channel, n) == ProtoFIFO || CH_PROTO(channel, n) == ProtoCharacter)
  {
    int result;
    while(CH_SOURCE(channel, n)->pos < channel->getpos)
    {
      char buf[BUFFER_SIZE];
      result = fread(buf, 1, channel->getpos - CH_SOURCE(channel, n)->pos,
          CH_HANDLE(channel, n));
      ZLOGFAIL(result < 0, EIO, "%s %d: %s", channel->alias, n, strerror(errno));
      CH_SOURCE(channel, n)->pos += result;
    }
  }

  /* if source is a network get over (*->getpos - *->pos) bytes */
  else if(IS_NETWORK(CH_PROTO(channel, n)))
  {
    while(CH_SOURCE(channel, n)->pos < channel->getpos && !channel->eof)
    {
      FetchMessage(channel, n);
      CH_SOURCE(channel, n)->pos += channel->bufend;
    }
  }

  /* no need to sync with regular files, just set (*->getpos to *->pos) */
  else
    CH_SOURCE(channel, n)->pos = channel->getpos;

  ZLOGS(LOG_DEBUG, "%s;%d skipped pos = %ld, getpos = %ld",
      channel->alias, n, CH_SOURCE(channel, n)->pos, channel->getpos);
  ZLOGFAIL(CH_SOURCE(channel, n)->pos != channel->getpos,
      EPIPE, "%s;%d is out of sync", channel->alias, n);
}

void PrefetchChannelCtor(struct ChannelDesc *channel, int n)
{
  int sock_type;
  struct Connection *c;

  assert(context != NULL);
  assert(channel != NULL);
  assert(n < channel->source->len);

  /* log parameters and channel internals */
  c = CH_SOURCE(channel, n);
  ZLOGS(LOG_DEBUG, "prefetch %s %d", channel->alias, n);

  /* choose socket type */
  ZLOGFAIL((uint32_t)CH_RW_TYPE(channel) - 1 > 1, EFAULT, "invalid i/o type");
  CH_FLAGS(channel, n) |= (CH_RW_TYPE(channel) - 1) << 1;
  sock_type = IS_RO(channel) ? ZMQ_PULL : ZMQ_PUSH;

  /* open source (0mq socket) */
  c->handle = zmq_socket(context, sock_type);
  ZLOGFAIL(c->handle == NULL, EFAULT,
      "cannot get socket for %s %d", channel->alias, n);

  /* allocate one message per channel */
  if(channel->msg == NULL)
  {
    channel->msg = g_malloc(sizeof(zmq_msg_t));
    ZMQ_FAIL(zmq_msg_init(channel->msg));
  }

  /* bind or connect the channel */
  sock_type == ZMQ_PULL ? Bind(channel, n) : Connect(channel, n);
}

void PrefetchChannelDtor(struct ChannelDesc *channel, int n)
{
  char url[BIG_ENOUGH_STRING]; /* debug purposes only */
  char buf[TAG_DIGEST_SIZE + 1] = {0}, *digest = buf;
  int dig_size = 0;

  /* skip source closing if session is broken */
  if(GetExitCode() != 0) return;

  assert(context != NULL);
  assert(channel != NULL);
  assert(n < channel->source->len);
  assert(channel->msg != NULL);
  assert(CH_HANDLE(channel, n) != NULL);

  /* log parameters and channel internals */
  MakeURL(channel, n, url, BIG_ENOUGH_STRING);
  ZLOGS(LOG_DEBUG, "closing %s with url %s", channel->alias, url);

  /* close WO source (send eof) */
  if(IS_WO(channel))
  {
    /* send 1st part of eof */
    ZMQ_FAIL(zmq_msg_init_data(channel->msg, digest, 0, NULL, NULL));
    SendMessage(channel, n);
    ZMQ_FAIL(zmq_msg_close(channel->msg));

    /* send the last part of eof */
    if(channel->tag != NULL)
    {
      TagDigest(channel->tag, digest);
      dig_size = TAG_DIGEST_SIZE;
    }
    ZMQ_FAIL(zmq_msg_init_data(channel->msg, digest, dig_size, NULL, NULL));
    SendMessage(channel, n);
    ZMQ_FAIL(zmq_msg_close(channel->msg));
    channel->eof = 1;
  }
  /* "fast forward" RO source until eof */
  else
  {
    /*
     * todo(d'b): this is a temporary solution. complete solution can be
     * implemented when daemon/proxy will be able to terminate zerovm by
     * request for instance when cluster is already done except "sending"
     * reserve node (reported as useless by "receiving" node(s))
     */
    if(CH_SOURCE(channel, n)->pos < channel->getpos)
    {
      channel->eof = 0;
      SyncSource(channel, n);
      FetchMessage(channel, n); /* reach EOF */
    }
    digest = MessageData(channel);
  }

  /* close source */
  ZMQ_FAIL(zmq_close(CH_HANDLE(channel, n)));
  CH_HANDLE(channel, n) = NULL;
  ZLOGS(LOG_DEBUG, "%s closed with tag %s", url, digest);
}

void FreeMessage(struct ChannelDesc *channel)
{
  assert(channel != NULL);

  if(channel->msg != NULL)
  {
    zmq_msg_close(channel->msg);
    g_free(channel->msg);
    channel->msg = NULL;
  }
}
