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
 * this design uses new protocol to send/receive data (for details:
 * https://gist.github.com/rpedde/8927215, by Ron Pedde). manifest should
 * contain "Broker" string (path to connect to the broker and send/receive
 * control information). also zerovm assumes that the manifest provider
 * does not provide information to broker
 *
 * 1. connect broker (where to get connection line/url?)
 * 2. send the broker the channel connection information
 * 3. get the broker's answer, extract the return code and pipe path
 * 4. repeat (2,3) until the channels list become empty
 * 5. use channels sending/receiving data
 * 6. close channels sending appropriate commands to the broker
 * 9. quit broker session
 *
 * questions:
 * 1. are we using same (control) channel to get/receive data for all network channels?
 * 2. will we use control channel to communicate with daemon?
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
#define QUEUE_SIZE 16
#define MAX_CONN 1
#define B_EOL "\n"
#define B_INIT_ANSWER "%d"
#define B_OK 200
#define B_QUIT "QUIT" B_EOL
#define B_QUIT_ANSWER "%d"
#define B_POPEN "POPEN %s %s %s" B_EOL
#define B_POPEN_ANSWER "%d %s"
#define B_PCLOSE "PCLOSE %s" B_EOL
#define B_PCLOSE_ANSWER "%d"
#define B_BUF_SIZE 1024 /* broker command buffer size */

static int control = -1; /* file descriptor to broker */
static int bsock = -1; /* only need to free socket resource after usage */

/* TODO(d'b): replace the function with inline error */
static char *getlasterror_desc()
{
  return strerror(errno);
}

/* open the channel */
/* ### change it */
static void Open(struct ChannelDesc *channel)
{
  struct sockaddr_un remote = {AF_UNIX, ""};

  ZLOG(LOG_DEBUG, "Bind: %s", channel->alias);
  ZLOGFAIL(bind(GPOINTER_TO_INT(channel->handle), &remote,
      sizeof remote) < 0, EIO, "bind: %s", strerror(errno));
  ZLOGFAIL(listen(GPOINTER_TO_INT(channel->handle),
      QUEUE_SIZE) < 0, EIO, "listen: %s", strerror(errno));
}

void NetCtor(const struct Manifest *manifest)
{
  struct sockaddr_un remote = {AF_UNIX, ""};
  socklen_t len = sizeof remote;
  char buffer[B_BUF_SIZE];
  int size;
  int code;

  /* open control channel to broker */
  bsock = socket(AF_UNIX, SOCK_STREAM, 0);
  ZLOGFAIL(bsock < 0, EIO, "%s", strerror(errno));
  strcpy(remote.sun_path, manifest->broker);
  ZLOGFAIL(bind(bsock, &remote, sizeof remote) < 0, EIO, "%s", strerror(errno));
  ZLOGFAIL(listen(bsock, QUEUE_SIZE) < 0, EIO, "%s", strerror(errno));

  /* accept connection to broker */
  control = accept(bsock, &remote, &len);
  ZLOGFAIL(control < 0, EIO, "%s", strerror(errno));

  /* command broker to serve */
  size = read(control, buffer, B_BUF_SIZE);
  sscanf(buffer, B_INIT_ANSWER, &code);
  ZLOGFAIL(code != B_OK, EIO, "broker returned error %d", code);
}

void NetDtor(struct Manifest *manifest)
{
  char buffer[B_BUF_SIZE];
  int code;

  /* log out broker */
  if(control >= 0)
  {
    write(control, B_QUIT, sizeof B_QUIT);
    sscanf(buffer, B_QUIT_ANSWER, &code);
    ZLOG(LOG_DEBUG, "broker closed with code %d", code);
    close(control);
  }

  if(bsock >= 0)
    close(bsock);
}

/*
 * TODO(d'b): decide interface
 */
static int GetCommand(struct ChannelDesc *channel)
{

}

/*
 * TODO(d'b): decide interface
 */
static int GetData(struct ChannelDesc *channel)
{

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

  Open(channel);

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
