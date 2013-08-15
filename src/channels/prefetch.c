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

#define LOWEST_AVAILABLE_PORT 49152
#define NET_BUFFER_SIZE 0x10000

/* todo: find more neat solution than put it twice */
#define XARRAY(a) static char *ARRAY_##a[] = {a};
#define X(a) #a,
  XARRAY(PROTOCOLS)
#undef X

static void *context = NULL; /* zeromq context */
static char control_digest[TAG_DIGEST_SIZE];

char *GetControlDigest()
{
  return control_digest;
}

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
  char buf[BIG_ENOUGH_STRING], *url = buf;
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
  int result;
  char buf[BIG_ENOUGH_STRING], *url = buf;
  uint64_t hwm = 1; /* high water mark to block on sending */

  result = zmq_setsockopt(CH_HANDLE(channel, n), ZMQ_HWM, &hwm, sizeof hwm);
  ZLOGFAIL(result != 0, EFAULT, "cannot set high water mark");

  MakeURL(channel, n, url, BIG_ENOUGH_STRING);
  ZLOGS(LOG_DEBUG, "connect url %s to %s %d", url, channel->alias, n);
  result = zmq_connect(CH_HANDLE(channel, n), url);
  ZLOGFAIL(result != 0, EFAULT, "cannot connect %s", channel->alias);
}

/*
 * check for an error. if encountered put it to log and
 * exit from the current function with standard error code
 * should be used in the nexe runtime instead of ZLOGFAIL()
 */
#define ZMQ_TEST_STATE(code, msg_ptr) \
    if((code) != 0)\
    {\
      ZLOG(LOG_ERROR, "zmq: error %d, %s",\
              zmq_errno(), zmq_strerror(zmq_errno()));\
      zmq_msg_close(msg_ptr);\
      return -1;\
    }

/*
 * initiate networking (if there are network channels)
 * note: will run only once on the 1st channel construction
 */
void NetCtor(const struct Manifest *manifest)
{
  /* get zmq context */
  context = zmq_init(1);
  ZLOGFAIL(context == NULL, EFAULT, "cannot initialize zeromq context");
}

/* finalize networking */
void NetDtor(struct Manifest *manifest)
{
  /* terminate context */
  zmq_term(context);
  context = NULL;
}

static void UpdateChannelState(struct ChannelDesc *channel, int n)
{
  int64_t more = 0;
  size_t more_size = sizeof more;
  struct Connection *c = CH_SOURCE(channel, n);

  zmq_getsockopt(c->handle, ZMQ_RCVMORE, &more, &more_size);

  /* etagged eof received */
  if(more != 0 && channel->bufend == TAG_DIGEST_SIZE && channel->tag != NULL)
  {
    memcpy(control_digest, zmq_msg_data(channel->msg), TAG_DIGEST_SIZE);

    /* receive the zero part */
    zmq_recv(c->handle, channel->msg, 0);
    channel->bufend = zmq_msg_size(channel->msg);
    ZLOGIF(channel->bufend != 0, "%s has invalid eof", channel->alias);
    channel->eof = 1;
  }

  /* simple eof received */
  if(more == 0 && channel->bufend == 0 && channel->tag == NULL)
    channel->eof = 1;
}

/* receive data from the network source */
int32_t FetchMessage(struct ChannelDesc *channel, int n, char *buf, int32_t count)
{
  int32_t readrest = count;
  int32_t toread = 0;

  assert(channel != NULL);
  assert(buf != NULL);
  assert(channel->bufpos <= NET_BUFFER_SIZE);
  assert(channel->bufend >= 0);
  assert(channel->bufend <= NET_BUFFER_SIZE);

  /*
   * read message part by part until "count" not reached.
   * the part size defined as 64kb
   */
  for(readrest = count; readrest > 0; readrest -= toread)
  {
    /* calculates available data, if needed update buffer and restart loop */
    toread = MIN(readrest, channel->bufend - channel->bufpos);
    if(channel->eof && toread == 0) break;

    if(toread == 0)
    {
      int result;

      /* re-initialize message and rewind the channel buffer */
      zmq_msg_close(channel->msg);
      channel->bufpos = 0;
      result = zmq_msg_init(channel->msg);
      ZMQ_TEST_STATE(result, channel->msg);

      /* read the next message */
      result = zmq_recv(CH_HANDLE(channel, n), channel->msg, 0);
      ZMQ_TEST_STATE(result, channel->msg);
      channel->bufend = zmq_msg_size(channel->msg);

      UpdateChannelState(channel, n);
      continue;
    }

    /* there is the data to take */
    memcpy(buf, (char*)zmq_msg_data(channel->msg) + channel->bufpos, toread);
    channel->bufpos += toread;
    buf += toread;
  }

  return count - readrest;
}

/* write to network source */
int32_t SendMessage(struct ChannelDesc *channel, int n, const char *buf, int32_t count)
{
  int result;
  int32_t writerest;
  int32_t flag;

  assert(channel != NULL);
  assert(buf != NULL);

  /* write EOF as a multi-part message if etag enabled */
  flag = channel->eof ? ZMQ_SNDMORE : 0;

  /* send a buffer through the multiple messages */
  for(writerest = count; writerest > 0; writerest -= NET_BUFFER_SIZE)
  {
    int32_t towrite = MIN(writerest, NET_BUFFER_SIZE);

    /*
     * since 0mq is not able to send the messages synchronously,
     * "zero copy" cannot be used
     * todo(d'b): limit the queue to 1 message, send given data in chunks
     *   as "zero copy" and copy the remaining chunk to buffer allocated
     *   by zerovm so it can be sent asynchronously
     */

    /* create the message */
    result = zmq_msg_init_size(channel->msg, towrite);
    ZMQ_TEST_STATE(result, channel->msg);
    memcpy(zmq_msg_data(channel->msg), buf, towrite);

    /* send the message */
    result = zmq_send(CH_HANDLE(channel, n), channel->msg, flag);
    ZMQ_TEST_STATE(result, channel->msg);

    /* destroy the message (caring about EOF) */
    buf += towrite;
    if(channel->eof == 0) zmq_msg_close(channel->msg);
  }

  /* if sending EOF */
  if(channel->eof)
  {
    /* create the message if didn't */
    if(count == 0)
    {
      result = zmq_msg_init(channel->msg);
      ZMQ_TEST_STATE(result, channel->msg);
    }

    result = zmq_msg_init_size(channel->msg, 0);
    ZMQ_TEST_STATE(result, channel->msg);
    result = zmq_send(CH_HANDLE(channel, n), channel->msg, 0);
    ZMQ_TEST_STATE(result, channel->msg);
    zmq_msg_close(channel->msg);
  }

  return count;
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

  /* explicitly reset network regarded fields */
  c->handle = NULL;
  channel->bufend = 0;
  channel->bufpos = 0;

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
    int result;
    channel->msg = g_malloc(sizeof(zmq_msg_t));
    result = zmq_msg_init(channel->msg);
    ZLOGFAIL(result != 0, EFAULT, "cannot init message");
  }

  /* bind or connect the channel */
  sock_type == ZMQ_PULL ? Bind(channel, n) : Connect(channel, n);
}

/* close network source */
void PrefetchChannelDtor(struct ChannelDesc *channel, int n)
{
  char url[BIG_ENOUGH_STRING]; /* debug purposes only */

  assert(context != NULL);
  assert(channel != NULL);
  assert(n < channel->source->len);

  /* log parameters and channel internals */
  MakeURL(channel, n, url, BIG_ENOUGH_STRING);
  ZLOGS(LOG_DEBUG, "closing %s with url %s", channel->alias, url);

  /* "eof" mounted source if session is not broken */
  if(channel->msg != NULL && CH_HANDLE(channel, n) != NULL)
  {
    /* close "PUT" source (send eof) */
    if(IS_WO(channel))
    {
      int dig_size = 0;
      char digest[TAG_DIGEST_SIZE + 1];

      /* prepare and send local digest */
      if(channel->tag != NULL)
      {
        TagDigest(channel->tag, digest);
        dig_size = TAG_DIGEST_SIZE;
      }
      channel->eof = 1;
      SendMessage(channel, n, digest, dig_size);
      ZLOGS(LOG_DEBUG, "%s closed with tag %s, putsize %ld",
          channel->alias, digest, channel->counters[PutSizeLimit]);

      /*
       * if the last message is not delivered send another one in blocking
       * mode. when unblocked, the last message sent and socket can be closed
       * todo: the patch can be removed when 0mq zmq_term will be fixed
       */
      do
      {
        struct Connection *c = CH_SOURCE(channel, n);
        int result;
        uint32_t flag = 0;
        size_t fsize = sizeof flag;

        /* check if there is unsent message */
        result = zmq_getsockopt(c->handle, ZMQ_EVENTS, &flag, &fsize);
        ZLOGIF(result == -1, "error closing %s %d", channel->alias, n);
        if(flag & ZMQ_POLLOUT) break;

        /* send empty message message in blocking mode */
        zmq_msg_init_size(channel->msg, 0);
        zmq_send(c->handle, channel->msg, 0);
      } while(0);
    }
  }

  /* free message only when the last source reached */
  if(channel->msg != NULL && channel->source->len - n == 1)
  {
    zmq_msg_close(channel->msg);
    g_free(channel->msg);
    channel->msg = NULL;
  }

  /* close source */
  if(CH_HANDLE(channel, n) != NULL)
    zmq_close(CH_HANDLE(channel, n));
}
