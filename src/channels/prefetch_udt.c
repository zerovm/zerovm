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
#include <netdb.h>
#include <udt/udtc.h>
#include "src/channels/prefetch.h"
#include "src/main/accounting.h"
#include "src/main/report.h"

#define NET_BUFFER_SIZE BUFFER_SIZE
#define ERROR(code) ZLOGIF(code, "failed: %s", udt_getlasterror_desc())
#define MAX_CONN 1

/*
 * accept "bind" channels. since udt_accept() is a blocking thing
 * the function should be used *only* after other channels are mounted
 * TODO(d'b): remove it after "channels" re-design
 */
void PrefetchAccept(struct ChannelDesc *channel)
{
  int n;
  int result;
  struct sockaddr_in incoming;

  /* skip inappropriate channels */
  if(!IS_RO(channel)) return;

  for(n = 0; n < channel->source->len; ++n)
  {
    if(IS_FILE(CH_CONN(channel, n))) continue;

    result = udt_accept(GPOINTER_TO_INT(CH_HANDLE(channel, n)),
        (struct sockaddr*)&incoming, &result);
    CH_HANDLE(channel, n) = GINT_TO_POINTER(result);
    ZLOGFAIL(GPOINTER_TO_INT(CH_HANDLE(channel, n)) == UDT_INVALID_SOCK, EFAULT,
        "bind %s;%d: %s", channel->alias, n, udt_getlasterror_desc());
  }
}

/* bind the RO source */
static void Bind(struct ChannelDesc *channel, int n, struct addrinfo *local)
{
  int result;
  struct sockaddr_in incoming;
  int i = sizeof incoming;

  ZLOG(LOG_DEBUG, "Bind: %s;%d", channel->alias, n);

  /* bind */
  result = udt_bind(GPOINTER_TO_INT(CH_HANDLE(channel, n)),
      local->ai_addr, local->ai_addrlen);
  ZLOGFAIL(result != 0, EFAULT, "bind %s;%d: %s",
      channel->alias, n, udt_getlasterror_desc());

  /* listen */
  result = udt_listen(GPOINTER_TO_INT(CH_HANDLE(channel, n)), MAX_CONN);
  ZLOGFAIL(result == UDT_ERROR, EFAULT, "listen: %s", udt_getlasterror_desc());
  CH_BACKUP(channel, n) = CH_HANDLE(channel, n);

  /* get port */
  result = udt_getsockname(GPOINTER_TO_INT(CH_HANDLE(channel, n)),
      (struct sockaddr*)&incoming, &i);
  ZLOGFAIL(result == UDT_ERROR, EFAULT, "getsockname: %s", udt_getlasterror_desc());
  CH_PORT(channel, n) = ntohs(incoming.sin_port);
}

/* connect the WO source */
static void Connect(struct ChannelDesc *channel, int n, struct addrinfo *local)
{
  char *ip;
  char *port;
  int result;
  struct addrinfo *peer;
  struct in_addr ip_int;

  ZLOG(LOG_DEBUG, "Connect: %s;%d", channel->alias, n);

  /* update connection information (do we really need it?) */
  ip_int.s_addr = CH_HOST(channel, n);
  ip = inet_ntoa(ip_int);
  port = g_strdup_printf("%d", CH_PORT(channel, n));
  ZLOGFAIL(getaddrinfo(ip, port, local, &peer) != 0, EFAULT,
      "%s;%d has incorrect network address", channel->alias, n);

  /* connect */
  result = udt_connect(GPOINTER_TO_INT(CH_HANDLE(channel, n)),
      peer->ai_addr, peer->ai_addrlen);
  freeaddrinfo(peer);
  ZLOGFAIL(result != 0, EFAULT, "connect %s;%d: %s",
      channel->alias, n, udt_getlasterror_desc());
}

void NetCtor(const struct Manifest *manifest)
{
  ZLOG(LOG_DEBUG, "NetCtor: udt");
  ZLOGFAIL(udt_startup() != 0, EFAULT, "udt: %s", udt_getlasterror_desc());
}

void NetDtor(struct Manifest *manifest)
{
  ZLOG(LOG_DEBUG, "NetDtor: udt");
  ZLOGFAIL(udt_cleanup() != 0, EFAULT, "udt: %s", udt_getlasterror_desc());
}

char *MessageData(struct ChannelDesc *channel)
{
  ZLOG(LOG_DEBUG, "MessageData: %s", channel->alias);
  return channel->msg;
}

void FreeMessage(struct ChannelDesc *channel)
{
  ZLOG(LOG_DEBUG, "FreeMessage: %s", channel->alias);
  if(IS_RO(channel)) g_free(channel->msg);
  channel->msg = NULL;
}

/* get the next message. updates channel->msg (and indices) */
static void GetMessage(struct ChannelDesc *channel, int n)
{
  ZLOGS(LOG_INSANE, "GetMessage: %s;%d", channel->alias, n);
  channel->bufpos = 0;
  channel->bufend = udt_recv(GPOINTER_TO_INT(CH_HANDLE(channel, n)),
      channel->msg, NET_BUFFER_SIZE, 0);

  /* got data */
  if(channel->bufend > 0) return;

  /* EOF */
  channel->eof = 1;
  channel->bufend = 0;
}

void FetchMessage(struct ChannelDesc *channel, int n)
{
  ZLOGS(LOG_INSANE, "FetchMessage: %s;%d", channel->alias, n);

  /* get message */
  if(!channel->eof)
    GetMessage(channel, n);
}

int32_t SendData(struct ChannelDesc *channel, int n, const char *buf, int32_t count)
{
  int i;
  int pos;

  assert(channel != NULL);
  assert(buf != NULL);
  assert(count >= 0);

  ZLOGS(LOG_INSANE, "SendData: channel %s;%d, buffer=0x%lx, size=%d",
      channel->alias, n, (intptr_t)buf, count);

  /* send a buffer through the multiple messages */
  for(pos = 0; pos < count; pos += i)
  {
    i = MIN(NET_BUFFER_SIZE, count - pos);
    i = udt_send(GPOINTER_TO_INT(CH_HANDLE(channel, n)), buf + pos, i, 0);
    ZLOGFAIL(i < 0, EFAULT, "send: %s", udt_getlasterror_desc());
  }

  return count;
}

void SyncSource(struct ChannelDesc *channel, int n)
{
  if(!CH_SEQ_READABLE(channel)) return;

  ZLOGS(LOG_INSANE, "SyncSource: %s;%d before skip pos = %ld, getpos = %ld",
      channel->alias, n, CH_CONN(channel, n)->pos, channel->getpos);

  /* if source is a pipe read (*->getpos - *->pos) bytes */
  if(CH_PROTO(channel, n) == ProtoFIFO || CH_PROTO(channel, n) == ProtoCharacter)
  {
    int result;
    while(CH_CONN(channel, n)->pos < channel->getpos)
    {
      char buf[BUFFER_SIZE];
      result = fread(buf, 1, channel->getpos - CH_CONN(channel, n)->pos,
          CH_HANDLE(channel, n));
      ZLOGFAIL(result < 0, EIO, "%s;%d: %s", channel->alias, n, strerror(errno));
      CH_CONN(channel, n)->pos += result;
    }
  }

  /* if source is a network get over (*->getpos - *->pos) bytes */
  else if(IS_NETWORK(CH_CONN(channel, n)))
  {
    while(CH_CONN(channel, n)->pos < channel->getpos && !channel->eof)
    {
      FetchMessage(channel, n);
      CH_CONN(channel, n)->pos += channel->bufend;
    }
  }

  /* no need to sync with regular files, just set (*->getpos to *->pos) */
  else
    CH_CONN(channel, n)->pos = channel->getpos;

  ZLOGS(LOG_INSANE, "%s;%d skipped pos = %ld, getpos = %ld",
      channel->alias, n, CH_CONN(channel, n)->pos, channel->getpos);
  ZLOGFAIL(CH_CONN(channel, n)->pos != channel->getpos,
      EPIPE, "%s;%d is out of sync", channel->alias, n);
}

void PrefetchChannelCtor(struct ChannelDesc *channel, int n)
{
  int i;
  char *ip;
  char *port;
  struct addrinfo hint = {0};
  struct addrinfo *local;
  struct in_addr ip_int;

  assert(channel != NULL);
  assert(n < channel->source->len);
  ZLOGS(LOG_DEBUG, "PrefetchChannelCtor %s;%d", channel->alias, n);

  /* choose socket type */
  ZLOGFAIL((uint32_t)CH_RW_TYPE(channel) - 1 > 1, EFAULT, "invalid i/o type");
  CH_FLAGS(channel, n) |= (CH_RW_TYPE(channel) - 1) << 1;

  /* get address structure via "hint" to "local" */
  hint.ai_flags = AI_PASSIVE;
  hint.ai_family = AF_INET;
  hint.ai_socktype = SOCK_STREAM;

  ip_int.s_addr = CH_HOST(channel, n);
  ip = IS_RO(channel) ? NULL : inet_ntoa(ip_int);
  port = g_strdup_printf("%d", CH_PORT(channel, n));
  ZLOGFAIL(getaddrinfo(ip, port, &hint, &local) != 0, EFAULT,
      "%s;%d has incorrect network address", channel->alias, n);

  /* open socket */
  i = udt_socket(local->ai_family, local->ai_socktype, local->ai_protocol);
  ZLOGFAIL(i == UDT_INVALID_SOCK, EFAULT, "%s;%d: %s",
      channel->alias, n, udt_getlasterror_desc());
  CH_HANDLE(channel, n) = GINT_TO_POINTER(i);

  /* do NOT reuse an existing address */
  i = 0;
  ZLOGFAIL(udt_setsockopt(GPOINTER_TO_INT(CH_HANDLE(channel, n)), 0, UDT_UDT_REUSEADDR,
      &i, sizeof i) == UDT_ERROR, EFAULT, "%s", udt_getlasterror_desc());

  /*
   * TODO(d'b): remove this when code will be re-factored to use the user
   * provided buffer
   */
  channel->msg = IS_RO(channel) ? g_malloc(NET_BUFFER_SIZE) : NULL;

  /* bind or connect the channel */
  IS_RO(channel) ? Bind(channel, n, local) : Connect(channel, n, local);
  freeaddrinfo(local);
}

void PrefetchChannelDtor(struct ChannelDesc *channel, int n)
{
  /* skip source closing if session is broken */
  if(GetExitCode() != 0) return;

  assert(channel != NULL);
  assert(n < channel->source->len);
  assert(CH_HANDLE(channel, n) != NULL);
  ZLOGS(LOG_DEBUG, "closing %s;%d", channel->alias, n);

  /*
   * TODO(d'b): this is the 1st version of UDT EOF. the channel source
   * does not have digest. the digest will be added later
   */
  /* close WO source (send EOF) */
  if(IS_WO(channel))
  {
    /* <...> EOF removed */
  }
  /* close RO source, "fast forward" to EOF if needed */
  else
  {
    /* <...> EOF and source "rewinding" removed */
    channel->eof = 1;
  }

  /* close source */
  udt_close(GPOINTER_TO_INT(CH_HANDLE(channel, n)));
  udt_close(GPOINTER_TO_INT(CH_BACKUP(channel, n)));
  CH_HANDLE(channel, n) = NULL;
  CH_BACKUP(channel, n) = NULL;
  ZLOGS(LOG_DEBUG, "%s;%d closed", channel->alias, n);
}
