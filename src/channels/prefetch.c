/*
 * preallocate network channel
 * note: made over zeromq library
 *
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
#include <zmq.h>
#include "src/main/manifest_parser.h"
#include "src/main/manifest_setup.h" /* todo(d'b): remove it. (system_manifest) */
#include "src/channels/prefetch.h"
#include "src/main/etag.h"
#include "src/main/nacl_globals.h" /* todo(d'b): remove it. (gnap) */
#include "src/channels/name_service.h"

static uint32_t channels_cnt = 0; /* needs for NetCtor/Dtor */
static void *context = NULL; /* zeromq context */
static uint32_t binds = 0; /* "bind" channels number */
static uint32_t connects = 0; /* "connect" channels number */

/* make url from the given record and return it through the "url" parameter */
static void MakeURL(char *url, int32_t size,
    const struct ChannelDesc *channel, const struct ChannelConnection *record)
{
  char host[BIG_ENOUGH_STRING];

  assert(url != NULL);
  assert(record != NULL);
  assert(channel != NULL);
  assert(channel->name != NULL);

  ZLOGFAIL(record->host == 0 && record->port != 0,
      EFAULT, "named hosts are not supported");
  ZLOGFAIL(size < 6, EFAULT, "too short url buffer");

  /* create string containing ip or id as the host name */
  switch(record->mark)
  {
    struct in_addr ip;
    case BIND_MARK:
      g_snprintf(host, BIG_ENOUGH_STRING, "*");
      break;
    case CONNECT_MARK:
    case OUTSIDER_MARK:
      ip.s_addr = bswap_32(record->host);
      g_snprintf(host, BIG_ENOUGH_STRING, "%s", inet_ntoa(ip));
      break;
    default:
      ZLOGFAIL(1, EFAULT, "unknown channel mark");
      break;
  }

  /* construct url */
  g_snprintf(url, size, "%s://%s:%u",
      StringizeChannelSourceType(record->protocol), host, record->port);

  ZLOG(LOG_INSANE, "url = %s", url);
}

/*
 * bind the given channel using info from netlist
 * returns not 0 if failed
 * note: replaces PrepareBind if no name service available
 */
static int DoBind(const struct ChannelDesc* channel)
{
  struct ChannelConnection *record;
  char buf[BIG_ENOUGH_STRING], *url = buf;

  record = GetChannelConnectionInfo(channel);
  assert(record != NULL);

  MakeURL(url, BIG_ENOUGH_STRING, channel, record);
  ZLOGS(LOG_DEBUG, "bind url %s", url);
  return zmq_bind(channel->socket, url);
}

/*
 * prepare "bind" channel information for the name service
 * note: should be called before name service invocation
 */
static void PrepareBind(const struct ChannelDesc *channel)
{
  struct ChannelConnection *record;
  static uint16_t port = LOWEST_AVAILABLE_PORT;
  int result = 1;

  assert(channel != NULL);

  /* update netlist with the connection info */
  StoreChannelConnectionInfo(channel);

  /* if no name service is available just use given url and return */
  if(!NameServiceSet())
  {
    result = DoBind(channel);
    ZLOGFAIL(result != 0, EFAULT, "cannot bind %s", channel->alias);
    return;
  }

  record = GetChannelConnectionInfo(channel);
  assert(record != NULL);

  /* in case channel has port assigned (full url) bind it and return */
  if(record->port != 0)
  {
    result = DoBind(channel);
    ZLOGFAIL(result != 0, EFAULT, "cannot bind %s", channel->alias);
    return;
  }

  /*
   * if channel does not have port pick up the port in the loop
   * todo(d'b): check upcoming zmq version for port 0 binding
   */
  assert(record != NULL);
  for(;port >= LOWEST_AVAILABLE_PORT; ++port)
  {
    record->port = port;
    result = DoBind(channel);
    if(result == 0) break;
  }

  ZLOGFAIL(result != 0, EFAULT ,"cannot get port to bind %s", channel->alias);
  ZLOGS(LOG_DEBUG, "host = %u, port = %u", record->host, record->port);
}

/*
 * connect the given channel using info from netlist
 * returns not 0 if failed
 * note: replaces PrepareConnect if no name service available
 */
static int DoConnect(struct ChannelDesc* channel)
{
  struct ChannelConnection *record;
  char buf[BIG_ENOUGH_STRING], *url = buf;

  record = GetChannelConnectionInfo(channel);
  assert(record != NULL);

  MakeURL(url, BIG_ENOUGH_STRING, channel, record);
  ZLOGS(LOG_DEBUG, "connect url %s", url);
  return zmq_connect(channel->socket, url);
}

/*
 * prepare "connect" channel information for the name service
 * note: will be called before name service invocation
 */
static void PrepareConnect(struct ChannelDesc* channel)
{
  int result;
  assert(channel != NULL);

  /* update netlist with the connection info */
  StoreChannelConnectionInfo(channel);

  /* if no name service is available just use given url and return */
  if(!NameServiceSet())
  {
    uint64_t hwm = 1; /* high water mark for PUSH socket to block on sending */

    result = zmq_setsockopt(channel->socket, ZMQ_HWM, &hwm, sizeof hwm);
    ZLOGFAIL(result != 0, EFAULT, "cannot set high water mark");

    result = DoConnect(channel);
    ZLOGFAIL(result != 0, EFAULT, "cannot connect %s", channel->alias);
  }
}

void KickPrefetchChannels(const struct NaClApp *nap)
{
  int i;

  /* quietly return if no name service specified */
  if(!NameServiceSet()) return;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /* exchange channel information with the name server */
  ResolveChannels(nap, binds, connects);

  /* make connections */
  for(i = 0; i < nap->system_manifest->channels_count; ++i)
  {
    int result;
    struct ChannelDesc *channel = &nap->system_manifest->channels[i];
    struct ChannelConnection *record;

    assert(channel != NULL);

    /* skip all channels except the network "connect" ones */
    if(channel->source != ChannelTCP) continue;

    /* get the channel connection information */
    record = GetChannelConnectionInfo(channel);
    assert(record != NULL);

    /* only the "connect" channels need to be processed */
    if(record->mark != CONNECT_MARK) continue;

    /* bind or connect the channel and look at result */
    result = DoConnect(channel);
    ZLOGFAIL(result != 0, EFAULT, "cannot connect socket to %s", channel->alias);
  }

  /*
   * temporary fix (the lost 1st messsage) to allow 0mq to complete
   * the connection procedure. 1 millisecond should be enough
   * todo(d'b): replace it with the channels readiness check
   */
  usleep(PREPOLL_WAIT);
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

#define CHECK_TRANSPORT(proto) \
  if(strncmp(url, prefix[proto], strlen(prefix[proto])) == 0) return proto;
enum ChannelSourceType GetChannelProtocol(const char *url)
{
  char *prefix[] = CHANNEL_SOURCE_PREFIXES;
  assert(url != NULL);

  CHECK_TRANSPORT(ChannelIPC);
  CHECK_TRANSPORT(ChannelTCP);
  CHECK_TRANSPORT(ChannelINPROC);
  CHECK_TRANSPORT(ChannelPGM);
  CHECK_TRANSPORT(ChannelEPGM);
  CHECK_TRANSPORT(ChannelUDP);
  return ChannelSourceTypeNumber;
}
#undef CHECK_TRANSPORT

/*
 * initiate networking (if there are network channels)
 * note: will run only once on the 1st channel construction
 */
static INLINE void NetCtor()
{
  /* context will be get at the very 1st call */
  if(channels_cnt++) return;

  /* get zmq context */
  context = zmq_init(1);
  ZLOGFAIL(context == NULL, EFAULT, "cannot initialize zeromq context");

  /* initialize name service */
  NameServiceCtor();
}

/*
 * close all "push" channels manually after EOF's sent
 * note: temporary fix for zmq_term(). can be removed after zeromq
 *   team will fix it.
 * note: global nap object has been used. but it is ok since the patch
 *   is temporary
 * todo(d'b): remove this function
 */
static INLINE void CloseChannels()
{
  extern struct NaClApp *gnap;
  struct ChannelDesc *channels = gnap->system_manifest->channels;
  int busy;

  do
  {
    int i;
    busy = 0;

    for(i = 0; i < gnap->system_manifest->channels_count; ++i)
    {
      struct ChannelDesc *channel = &channels[i];
      if(channel->source == ChannelTCP && channel->limits[PutsLimit] != 0
          && channel->limits[PutSizeLimit] != 0)
      {
        uint32_t more;
        size_t more_size = sizeof more;

        if(channel->socket == NULL) continue;

        zmq_getsockopt(channel->socket, ZMQ_EVENTS, &more, &more_size);
        busy |= more != ZMQ_POLLOUT;
        if(more == ZMQ_POLLOUT)
        {
          zmq_close(channel->socket);
          channel->socket = NULL;
        }
      }
    }
  } while(busy);
}

/*
 * finalize networking (if there are network channels)
 * note: will run only once. should be called from channel destructor
 */
static INLINE void NetDtor()
{
  /* context will be destroyed at the last call */
  if(--channels_cnt) return;

  /* temporary fix to make zmq_term() working */
  CloseChannels();

  /* terminate context */
  zmq_term(context);

  /* release name service */
  NameServiceDtor();
}

int PrefetchChannelCtor(struct ChannelDesc *channel)
{
  int sock_type;

  /* check for the design errors */
  assert(channel != NULL);
  assert(channel->source == ChannelTCP);

  /* log parameters and channel internals */
  ZLOGS(LOG_DEBUG, "prefetch %s", channel->alias);

  /* explicitly reset network regarded fields */
  channel->socket = NULL;
  channel->bufend = 0;
  channel->bufpos = 0;

  /* open zmq socket. will run only 1st time */
  NetCtor();

  /* choose connection type and open socket */
  sock_type = channel->limits[GetsLimit]
      && channel->limits[GetSizeLimit] ? ZMQ_PULL : ZMQ_PUSH;
  channel->socket = zmq_socket(context, sock_type);
  ZLOGFAIL(channel->socket == NULL, EFAULT,
      "cannot get socket for %s", channel->alias);

  /* bind or connect the channel */
  if(sock_type == ZMQ_PUSH)
  {
    PrepareConnect(channel);
    ++connects;
  }
  else
  {
    int result = zmq_msg_init(&channel->msg);
    ZMQ_TEST_STATE(result, &channel->msg);
    PrepareBind(channel);
    ++binds;
  }

  return 0;
}

/* check and update channel EOF state and etag */
static INLINE void UpdateChannelState(struct ChannelDesc *channel)
{
  int64_t more = 0;
  size_t more_size = sizeof more;

  zmq_getsockopt(channel->socket, ZMQ_RCVMORE, &more, &more_size);

  /* etag enabled */
  if(more != 0 && channel->bufend == TAG_DIGEST_SIZE - 1 && CHANNELS_ETAG_ENABLED)
  {
    /* store received digest */
    memcpy(channel->control, zmq_msg_data(&channel->msg), TAG_DIGEST_SIZE - 1);

    /* receive the zero part */
    zmq_recv(channel->socket, &channel->msg, 0);
    channel->bufend = zmq_msg_size(&channel->msg);
    if(channel->bufend == 0) channel->eof = 1;
    else ZLOG(LOG_ERROR, "invalid eof detected on %s", channel->alias);
  }

  /* etag disabled */
  if(more == 0 && channel->bufend == 0 && !TagEngineEnabled())
    channel->eof = 1;
}

int32_t FetchMessage(struct ChannelDesc *channel, char *buf, int32_t count)
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
      zmq_msg_close(&channel->msg);
      channel->bufpos = 0;
      result = zmq_msg_init(&channel->msg);
      ZMQ_TEST_STATE(result, &channel->msg);

      /* read the next message */
      result = zmq_recv(channel->socket, &channel->msg, 0);
      ZMQ_TEST_STATE(result, &channel->msg);
      channel->bufend = zmq_msg_size(&channel->msg);

      UpdateChannelState(channel);
      continue;
    }

    /* there is the data to take */
    memcpy(buf, (char*)zmq_msg_data(&channel->msg) + channel->bufpos, toread);
    channel->bufpos += toread;
    buf += toread;
  }

  return count - readrest;
}

int32_t SendMessage(struct ChannelDesc *channel, const char *buf, int32_t count)
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
    result = zmq_msg_init_size(&channel->msg, towrite);
    ZMQ_TEST_STATE(result, &channel->msg);
    memcpy(zmq_msg_data(&channel->msg), buf, towrite);

    /* send the message */
    result = zmq_send(channel->socket, &channel->msg, flag);
    ZMQ_TEST_STATE(result, &channel->msg);

    /* destroy the message (caring about EOF) */
    buf += towrite;
    if(channel->eof == 0) zmq_msg_close(&channel->msg);
  }

  /* if sending EOF */
  if(channel->eof)
  {
    /* create the message if didn't */
    if(count == 0)
    {
      result = zmq_msg_init(&channel->msg);
      ZMQ_TEST_STATE(result, &channel->msg);
    }

    result = zmq_msg_init_size(&channel->msg, 0);
    ZMQ_TEST_STATE(result, &channel->msg);
    result = zmq_send(channel->socket, &channel->msg, 0);
    ZMQ_TEST_STATE(result, &channel->msg);
    zmq_msg_close(&channel->msg);
  }

  return count;
}

int PrefetchChannelDtor(struct ChannelDesc *channel)
{
  char url[BIG_ENOUGH_STRING];  /* debug purposes only */

  assert(channel != NULL);
  assert(channel->socket != NULL);

  /* log parameters and channel internals */
  MakeURL(url, BIG_ENOUGH_STRING, channel, GetChannelConnectionInfo(channel));
  ZLOGS(LOG_DEBUG, "%s has url %s", channel->alias, url);

  /* close "PUT" channel */
  if(channel->limits[PutsLimit] && channel->limits[PutSizeLimit])
  {
    int size = CHANNELS_ETAG_ENABLED ? TAG_DIGEST_SIZE - 1 : 0;

    /* prepare digest */
    if(TagEngineEnabled())
    {
      TagDigest(channel->tag, channel->digest);
      TagDtor(channel->tag);
    }

    /* send eof */
    channel->eof = 1;
    SendMessage(channel, channel->digest, size);
    ZLOGS(LOG_DEBUG, "%s closed with tag %s, putsize %ld",
        channel->alias, channel->digest, channel->counters[PutSizeLimit]);
  }

  /* close "GET" channel */
  if(channel->limits[GetsLimit] && channel->limits[GetSizeLimit])
  {
    /* wind the channel to the end */
    while(channel->eof == 0)
    {
      char buf[NET_BUFFER_SIZE];
      int32_t size = FetchMessage(channel, buf, NET_BUFFER_SIZE);
      ++channel->counters[GetsLimit];
      channel->counters[GetSizeLimit] += size;

      /* update tag if enabled */
      if(TagEngineEnabled())
        TagUpdate(channel->tag, buf, size);
    }

    /* test integrity (if etag enabled) */
    if(TagEngineEnabled())
    {
      /* prepare digest */
      TagDigest(channel->tag, channel->digest);
      TagDtor(channel->tag);

      /* raise the error if the data corrupted */
      if(memcmp(channel->control, channel->digest, TAG_DIGEST_SIZE) != 0)
      {
        ZLOG(LOG_ERROR, "%s corrupted, control: %s, local: %s",
            channel->alias, channel->control, channel->digest);
        SetExitState("data corrupted");
        SetExitCode(EPIPE);
      }

      ZLOGS(LOG_DEBUG, "%s closed with tag %s, getsize %ld",
          channel->alias, channel->digest, channel->counters[GetSizeLimit]);
    }

    zmq_msg_close(&channel->msg);
    zmq_close(channel->socket);
  }

  /* will destroy context and netlist after all network channels closed */
  NetDtor();

  return 0;
}
