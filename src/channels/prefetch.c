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

/*
 * WARNING: under construction!
 */

#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h> /* convert ip <-> int */
#include <netdb.h>
#include "src/channels/prefetch.h"
#include "src/main/accounting.h"
#include "src/main/report.h"

#define NET_BUFFER_SIZE BUFFER_SIZE
// ###
#define ERROR(code) ZLOGIF(code, "failed: %s", getlasterror_desc())
#define MAX_CONN 1

// ### replace function "in place"
static char *getlasterror_desc()
{
  return strerror(errno);
}

///*
// * accept "bind" channels. since accept() is a blocking thing
// * the function should be used *only* after other channels are mounted
// * TODO(d'b): remove it after "channels" re-design
// */
//void PrefetchAccept(struct ChannelDesc *channel)
//{
////  int n;
//  int result;
//  struct sockaddr_in incoming;
//
//  // ### assert if channel has "network" type
//
//  /* skip inappropriate channels */
//  if(!IS_RO(channel)) return;
//
////  for(n = 0; n < channel->source->len; ++n)
////  {
////    if(IS_FILE(CH_CONN(channel, n))) continue;
////
////    result = accept(GPOINTER_TO_INT(CH_HANDLE(channel, n)),
////        (struct sockaddr*)&incoming, (uint32_t*)&result);
////    CH_HANDLE(channel, n) = GINT_TO_POINTER(result);
////    ZLOGFAIL(GPOINTER_TO_INT(CH_HANDLE(channel, n)) == -1, EFAULT,
////        "bind %s;%d: %s", channel->alias, n, getlasterror_desc());
////  }
//  result = accept(GPOINTER_TO_INT(channel->handle),
//      (struct sockaddr*)&incoming, (uint32_t*)&result);
//  channel->handle = GINT_TO_POINTER(result);
//  ZLOGFAIL(GPOINTER_TO_INT(channel->handle) == -1, EFAULT,
//      "bind %s: %s", channel->alias, getlasterror_desc());
//}

/* bind the RO source */
// ### rewrite
static void Bind(struct ChannelDesc *channel, struct addrinfo *local)
{
//  int result;
//  struct sockaddr_in incoming;
//  int i = sizeof incoming;

  ZLOG(LOG_DEBUG, "Bind: %s", channel->alias);

//  /* bind */
//  result = bind(GPOINTER_TO_INT(channel->handle),
//      local->ai_addr, local->ai_addrlen);
//  ZLOGFAIL(result != 0, EFAULT, "bind %s: %s",
//      channel->alias, getlasterror_desc());
//
//  /* listen */
//  result = listen(GPOINTER_TO_INT(channel->handle), MAX_CONN);
//  ZLOGFAIL(result == -1, EFAULT, "listen: %s", getlasterror_desc());
}

/* connect the WO source */
// ### rewrite
static void Connect(struct ChannelDesc *channel, struct addrinfo *local)
{
//  char *ip;
//  char *port;
//  int result;
//  struct addrinfo *peer;
//  struct in_addr ip_int;

  ZLOG(LOG_DEBUG, "Connect: %s", channel->alias);

//  /* update connection information (do we really need it?) */
//  ip_int.s_addr = CH_HOST(channel, n);
//  ip = inet_ntoa(ip_int);
//  port = g_strdup_printf("%d", CH_PORT(channel, n));
//  ZLOGFAIL(getaddrinfo(ip, port, local, &peer) != 0, EFAULT,
//      "%s;%d has incorrect network address", channel->alias, n);
//
//  /* connect */
//  result = connect(GPOINTER_TO_INT(CH_HANDLE(channel, n)),
//      peer->ai_addr, peer->ai_addrlen);
//  freeaddrinfo(peer);
//  ZLOGFAIL(result != 0, EFAULT, "connect %s;%d: %s",
//      channel->alias, n, getlasterror_desc());
}

void NetCtor(const struct Manifest *manifest)
{
  // ### remove it completely
}

void NetDtor(struct Manifest *manifest)
{
  // ### remove it completely
}

// ### rewrite
int32_t FetchData(struct ChannelDesc *channel, char *buf, int32_t size)
{
  int32_t result = 0;

  /*
   * ->bufpos contains read size to workaround problem with
   * incomplete messages (usual case for udt).
   */
  assert(channel->bufpos <= NET_BUFFER_SIZE);
  assert(channel->bufpos > 0);
  ZLOGS(LOG_INSANE, "FetchMessage: %s", channel->alias);

  /* get message */
  if(!channel->eof)
  {
    channel->bufpos = 0;
    channel->bufend = 0;

    while(channel->bufend < size && !channel->eof)
    {
      int i = recv(GPOINTER_TO_INT(channel->handle),
          channel->msg + channel->bufend, size - channel->bufend, 0);
      if(i < 0) break; // ### fail here
      else
        channel->bufend += i;
      result += i;
    }

    /* only set EOF if this read returned nothing */
    if(channel->bufend == 0)
      channel->eof = 1;
  }

  return result;
}

// ### rewrite
int32_t SendData(struct ChannelDesc *channel, const char *buf, int32_t count)
{
  int i;
  int pos;

  assert(channel != NULL);
  assert(buf != NULL);
  assert(count >= 0);

  ZLOGS(LOG_INSANE, "SendData: channel %s;%d, buffer=0x%lx, size=%d",
      channel->alias, (intptr_t)buf, count);

  /* send a buffer through the multiple messages */
  for(pos = 0; pos < count; pos += i)
  {
    i = MIN(NET_BUFFER_SIZE, count - pos);
    i = send(GPOINTER_TO_INT(channel->handle), buf + pos, i, 0);
    ZLOGFAIL(i < 0, EFAULT, "send: %s", getlasterror_desc());
  }

  return count;
}

// ### rewrite
void PrefetchChannelCtor(struct ChannelDesc *channel)
{
//  int i;
////  char *ip;
////  char *port;
////  struct addrinfo hint = {0};
//  struct addrinfo *local;
////  struct in_addr ip_int;

  assert(channel != NULL);
  ZLOGS(LOG_DEBUG, "PrefetchChannelCtor %s", channel->alias);

  // ### to remove {{
  Bind(channel, NULL);
  Connect(channel, NULL);
  // }}

//  /* choose socket type */
//  ZLOGFAIL((uint32_t)CH_RW_TYPE(channel) - 1 > 1, EFAULT, "invalid i/o type");
//  channel->flags |= (CH_RW_TYPE(channel) - 1) << 1;

//  /* get address structure via "hint" to "local" */
//  hint.ai_flags = AI_PASSIVE;
//  hint.ai_family = AF_INET;
//  hint.ai_socktype = SOCK_STREAM;

//  ip_int.s_addr = CH_HOST(channel, n);
//  ip = IS_RO(channel) ? NULL : inet_ntoa(ip_int);
//  port = g_strdup_printf("%d", CH_PORT(channel, n));
//  ZLOGFAIL(getaddrinfo(ip, port, &hint, &local) != 0, EFAULT,
//      "%s;%d has incorrect network address", channel->alias, n);

//  /* open socket */
//  i = socket(local->ai_family, local->ai_socktype, local->ai_protocol);
//  ZLOGFAIL(i == -1, EFAULT, "%s: %s", channel->alias, getlasterror_desc());
//  channel->handle = GINT_TO_POINTER(i);
//
//  /* do NOT reuse an existing address */
//  i = 0;
//  ZLOGFAIL(setsockopt(GPOINTER_TO_INT(channel->handle), 0, SO_REUSEADDR,
//      &i, sizeof i) == -1, EFAULT, "%s", getlasterror_desc());

  /*
   * TODO(d'b): remove this when code will be re-factored to use the user
   * provided buffer
   */
//  channel->msg = IS_RO(channel) ? g_malloc(NET_BUFFER_SIZE) : NULL;
//
//  /* bind or connect the channel */
//  IS_RO(channel) ? Bind(channel, local) : Connect(channel, local);
//  freeaddrinfo(local);
}

void PrefetchChannelDtor(struct ChannelDesc *channel)
{
  /* skip source closing if session is broken */
  if(GetExitCode() != 0) return;

  assert(channel != NULL);
  assert(channel->handle != NULL);
  ZLOGS(LOG_DEBUG, "closing %s", channel->alias);

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
  close(GPOINTER_TO_INT(channel->handle));
  channel->handle = NULL;
  ZLOGS(LOG_DEBUG, "%s closed", channel->alias);
}
