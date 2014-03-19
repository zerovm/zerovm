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
 * this design based on the (modified) protocol by Ron Pedde (for details:
 * https://gist.github.com/rpedde/8927215). manifest should contain "Broker"
 * string (path to connect to the broker and send/receive control
 * information). manifest should contain "Node" as well
 *
 * 1. connect broker
 * 2. send the broker the channel connection information
 * 3. get the answer, extract the return code and if ok connect the channel
 * 4. repeat (2,3) until the channels list become empty
 * 5. use channels to serve user session
 * 6. close channels sending appropriate commands to the broker
 * 7. quit broker session
 *
 * note: in this version node is a string. therefore any id can be used in
 * "Node" and "Channel" (however it is advised to use 64-bit values)
 *
 * TODO(d'b): add complete design to /doc/networking.txt (update channels.txt,
 * manifest.txt; remove name_server.txt)
 * TODO(d'b): rewrite code removing code doubling. remove extra asserts
 */
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "src/channels/prefetch.h"
#include "src/main/report.h"

#define B_BUF_SIZE 1024 /* broker command buffer size */
#define B_EOL "\n"
#define B_OK 200

/* the format strings are here just for convenience */
#define B_INIT_ANSWER "%d"
#define B_QUIT "QUIT" B_EOL
#define B_QUIT_ANSWER "%d"
#define B_POPEN "POPEN %s %s %c" B_EOL
#define B_POPEN_ANSWER "%d"
#define B_PCLOSE "PCLOSE %s %s" B_EOL
#define B_PCLOSE_ANSWER "%d"

static char *node = NULL;
static int control = -1; /* file descriptor to broker */
static int bsock = -1; /* only need to free socket resource after usage */
static struct sockaddr_un broker = {AF_UNIX, ""};

void NetCtor(const struct Manifest *manifest)
{
  char buffer[B_BUF_SIZE];
  int code;

  /* don't proceed if broker is not specified */
  if(manifest->broker == NULL) return;

  ZLOGS(LOG_INSANE, "connecting to broker %s", manifest->broker);

  /* store node */
  node = manifest->node;

  /* open control channel to broker */
  code = UNIX_PATH_MAX - strlen(manifest->broker);
  ZLOGFAIL(code < 0, EFAULT, "too long broker path");
  bsock = socket(AF_UNIX, SOCK_STREAM, 0);
  ZLOGFAIL(bsock < 0, EIO, "%s", strerror(errno));
  strcpy(broker.sun_path, manifest->broker);

  /* connect to broker */
  control = connect(bsock, &broker, sizeof broker);
  ZLOGFAIL(control < 0, EIO, "%s", strerror(errno));

  /* command broker to serve */
  code = read(control, buffer, B_BUF_SIZE);
  ZLOGFAIL(code < 0, EIO, "control channel read error: %s", strerror(errno));
  sscanf(buffer, B_INIT_ANSWER, &code);
  ZLOGFAIL(code != B_OK, EIO, "broker returned error %d", code);

  ZLOGS(LOG_DEBUG, "broker %s connected", manifest->broker);
}

void NetDtor(struct Manifest *manifest)
{
  char buffer[B_BUF_SIZE];
  int code;

  /* don't proceed if broker is not specified */
  if(manifest->broker == NULL) return;

  ZLOGS(LOG_INSANE, "disconnecting broker %s", manifest->broker);

  /* log out broker and close control channel */
  if(control >= 0)
  {
    code = write(control, B_QUIT, sizeof B_QUIT);
    ZLOGIF(code < 0, "control channel write error: %s", strerror(errno));
    sscanf(buffer, B_QUIT_ANSWER, &code);
    ZLOG(LOG_DEBUG, "broker closed with code %d", code);
    close(control);
  }

  /* close control socket */
  if(bsock >= 0)
    close(bsock);

  ZLOGS(LOG_DEBUG, "broker %s disconnected", manifest->broker);
}

int32_t FetchData(struct ChannelDesc *channel, char *buf, int32_t size)
{
  int32_t result = 0;
  int32_t rest;
  int32_t toread;

  assert(channel != NULL);
  assert(channel->handle != NULL);
  assert(buf != NULL);

  ZLOGS(LOG_INSANE, "FetchData(%s, %p, %d)", channel->alias, buf, size);

  /* read to buffer with small chunks */
  for(rest = size; rest > 0; rest -= result)
  {
    toread = MIN(rest, BUFFER_SIZE);
    result = read(GPOINTER_TO_INT(channel->handle), buf, toread);

    if(result < 0) return -errno;
    if(result == 0) break;
    buf += result;
  }

  return size - rest;
}

/*
 * TODO(d'b): remove code doubling
 */
int32_t SendData(struct ChannelDesc *channel, const char *buf, int32_t size)
{
  int32_t result = 0;
  int32_t rest;
  int32_t tosend;

  assert(channel != NULL);
  assert(channel->handle != NULL);
  assert(buf != NULL);

  ZLOGS(LOG_INSANE, "SendData(%s, %p, %d)", channel->alias, buf, size);

  /* write from buffer with small chunks */
  for(rest = size; rest > 0; rest -= result)
  {
    tosend = MIN(rest, BUFFER_SIZE);
    result = write(GPOINTER_TO_INT(channel->handle), buf, tosend);

    if(result < 0) return -errno;
    if(result == 0) break;
    buf += result;
  }

  return size - rest;
}

void PrefetchChannelCtor(struct ChannelDesc *channel)
{
  char buffer[B_BUF_SIZE];
  int size;
  int code;

  assert(channel != NULL);
  assert(control >= 0);
  assert(bsock >= 0);
  assert(node != NULL);
  assert(!CH_RND_READABLE(channel));
  assert(!CH_RND_WRITEABLE(channel));
  assert(!CH_SEQ_READABLE(channel) || !CH_SEQ_WRITEABLE(channel));

  ZLOGS(LOG_INSANE, "opening channel %s", channel->alias);

  /* ask the broker to connect session to another */
  size = g_snprintf(buffer, B_BUF_SIZE, B_POPEN, node,
      channel->name, CH_SEQ_READABLE(channel) ? 'R' : 'W');
  code = write(control, buffer, size);
  ZLOGFAIL(size != code, EIO, "control channel write error");

  /* get broker's answer (is it OK to connect new channel) */
  size = read(control, buffer, B_BUF_SIZE);
  ZLOGFAIL(size < 0, EIO, "control channel read error: %s", strerror(errno));
  sscanf(buffer, B_POPEN_ANSWER, &code);
  ZLOGFAIL(code != B_OK, EIO, "broker returned error %d", code);

  /* open channel */
  code = connect(bsock, &broker, sizeof broker);
  ZLOGFAIL(code < 0, EIO, "%s", strerror(errno));
  channel->handle = GINT_TO_POINTER(code);
  ZLOGS(LOG_DEBUG, "%s opened successfully", channel->alias);
}

/*
 * TODO(d'b): remove code doubling
 */
void PrefetchChannelDtor(struct ChannelDesc *channel)
{
  char buffer[B_BUF_SIZE];
  int size;
  int code;

  /* prevent abort if channel is not constructed */
  if(channel == NULL) return;
  if(channel->alias == NULL) return;
  if(channel->handle == NULL) return;

  assert(control >= 0);
  assert(bsock >= 0);
  assert(node != NULL);
  assert(!CH_RND_READABLE(channel));
  assert(!CH_RND_WRITEABLE(channel));
  assert(!CH_SEQ_READABLE(channel) || !CH_SEQ_WRITEABLE(channel));

  ZLOGS(LOG_INSANE, "closing %s", channel->alias);

  /* ask the broker to connect session to another */
  size = g_snprintf(buffer, B_BUF_SIZE, B_PCLOSE, node, channel->name);
  code = write(control, buffer, size);
  ZLOGIF(size != code, "control channel write error");

  /* get broker's answer (is it OK to connect new channel) */
  size = read(control, buffer, B_BUF_SIZE);
  ZLOGIF(size < 0, "control channel read error: %s", strerror(errno));
  sscanf(buffer, B_PCLOSE_ANSWER, &code);
  ZLOGIF(code != B_OK, "broker returned error %d", code);

  /* close source */
  close(GPOINTER_TO_INT(channel->handle));
  channel->handle = NULL;
  ZLOGS(LOG_DEBUG, "%s closed", channel->alias);
}
