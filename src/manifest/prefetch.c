/*
 * preallocate network channel
 * note: made over zeromq library
 *
 * based on code created by YaroslavLitvinov
 * updated on: Dec 5, 2011
 *     Author: d'b
 */
#include <assert.h>
#include <stdio.h>
#include <zmq.h>
#include "api/zvm.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/manifest/trap.h"
#include "src/manifest/manifest_parser.h"
#include "src/manifest/mount_channel.h"
#include "src/manifest/prefetch.h"
#include "src/service_runtime/nacl_config.h"
#include "src/service_runtime/sel_mem.h"
#include "src/service_runtime/nacl_memory_object.h"
#include "src/platform/nacl_log.h"

/*
 * check for an error. if encountered put it to log and
 * exit from the current function with standard error code
 */
#define ZMQ_TEST_STATE(code, msg_ptr) \
    if((code) != 0)\
    {\
      NaClLog(LOG_ERROR, "zmq: error %d, %s\n",\
              zmq_errno(), zmq_strerror(zmq_errno()));\
      if(msg_ptr != NULL) zmq_msg_close(msg_ptr);\
      return ERR_CODE;\
    }

static void* context = NULL; /* zeromq context */
static int64_t channels_cnt = 0; /* network channels counter */

/* deallocator for the zeromq "zero-copy" send() design */
static void ZMQFree (void *data, void *hint)
{
  /*
   * doesn't deallocate anything. the given buffer is a user property
   * and the user will care about freeing it.
   */
  UNREFERENCED_PARAMETER(data);
  UNREFERENCED_PARAMETER(hint);
}

/*
 * initiate networking (if there are network channels)
 * note: will run only once. should be called from channel constructor
 */
static void NetCtor()
{
  /* context will be get at the very 1st call */
  if(channels_cnt++) return;

  /* get zmq context */
  context = zmq_init(1);
}

/*
 * finalize networking (if there are network channels)
 * note: will run only once. should be called from channel destructor
 */
static void NetDtor()
{
  /* context will be destroyed at the last call */
  if(--channels_cnt) return;

  /* terminate context */
  zmq_term(context);
}

/* allocate network channel */
int PrefetchChannelCtor(struct ChannelDesc* channel)
{
  int result;

  assert(channel != NULL);

  /* explicitly reset network regarded fields */
  channel->socket = NULL;
  channel->buffer = NULL;
  channel->bufend = 0;
  channel->bufpos = 0;

  /* open zmq socket */
  NetCtor(); /* in fact will run only 1st time */

  /*
   * 0mq doesn't containt p2p pattern. so we assume reading peer -
   * server and writing peer - client contingently.
   * note: the logic is still under construction. after the design will
   * be finished the code doubling should removed
   */

  /* assume it server */
  if(CHANNEL_SEQ_READABLE(channel) && CHANNEL_READABLE(channel))
  {
    channel->socket = zmq_socket(context, ZMQ_PULL);
    COND_ABORT(channel->socket == NULL, "cannot obtain socket");

    /*
     * connect to the given address
     * note: for ipc transport socket file should exist
     */
    result = zmq_connect(channel->socket, channel->name);
    COND_ABORT(result != 0, "cannot connect 'pull' socket to address");

    /*
     * allocate memory to hold message and initialize message 1st time
     * (it will be closed on the 1st ReadSocket() invocation)
     */
    channel->msg = malloc(sizeof *channel->msg);
    COND_ABORT(channel->msg == NULL, "cannot allocate memory to hold message");
    result = zmq_msg_init(channel->msg);
  }

  /* assume it client */
  if(CHANNEL_SEQ_WRITEABLE(channel) && CHANNEL_WRITEABLE(channel))
  {
    channel->socket = zmq_socket(context, ZMQ_PUSH);
    COND_ABORT(channel->socket == NULL, "cannot obtain socket");

    /* connect to the given address */
    result = zmq_bind(channel->socket, channel->name);
    COND_ABORT(result != 0, "cannot connect 'push' socket to address");
  }

  return OK_CODE;
}

/* finalize and deallocate network channel */
int PrefetchChannelDtor(struct ChannelDesc* channel)
{
  int result;

  assert(channel != NULL);
  assert(channel->socket != NULL);

  /* send EOF if the channel is writable */
  if(channel->limits[PutsLimit] && channel->limits[PutsLimit])
  {
    zmq_msg_t msg;

    /* the last send must be non-blocking or it will never complete */
    /* todo(): fix the bug "Resource temporarily unavailable" when send */
    if(zmq_msg_init(&msg) == 0)
    {
      result = zmq_msg_init_size(&msg, 0);
      result = zmq_send(channel->socket, &msg, ZMQ_NOBLOCK);
      result = zmq_msg_close(&msg);
    }
  }

  if(channel->limits[GetsLimit] && channel->limits[GetsLimit])
  {
    /* make zeromq deallocate used resources */
    zmq_msg_close(channel->msg);
    free(channel->msg);
  }

  /* close zmq socket */
  result = zmq_close(channel->socket);
  ZMQ_TEST_STATE(result, NULL);

  /* will destroy context after all network channels closed */
  NetDtor();

  return OK_CODE;
}

/*
 * get the next portion to the channel buffer, reset buffer indices
 * return the number of read bytes or negative error code
 */
static int32_t ReadSocket(struct ChannelDesc *channel)
{
  int result;
  int64_t more;
  size_t more_size = sizeof more;

  assert(channel != NULL);

  /* rewind the channel buffer and mark that no data is available */
  channel->bufpos = 0;
  channel->bufend = 0;

  /* Create an empty ZMQ message to hold the message part */
  zmq_msg_close(channel->msg); /* cheat. don't close the message until it empty */
  result = zmq_msg_init(channel->msg);
  ZMQ_TEST_STATE(result, channel->msg);

  /* Block until a message is available to be received from socket */
  result = zmq_recv(channel->socket, channel->msg, 0);
  ZMQ_TEST_STATE(result, channel->msg);

  /* Determine if more message parts are to follow */
  result = zmq_getsockopt(channel->socket, ZMQ_RCVMORE, &more, &more_size);
  ZMQ_TEST_STATE(result, channel->msg);

  /* store data pointer and size */
  /* special case: EOF. if zero length message received */
  channel->buffer = zmq_msg_data(channel->msg);
  channel->bufend = zmq_msg_size(channel->msg);

  /*
   * detect the end of channel (NET_EOF). zero size of received
   * buffer will be used as the mark of the end since current zmq
   * version socket close don't fit our needs
   */
  if(channel->bufend == 0) channel->bufpos = NET_EOF;

  return channel->bufend;
}

/*
 * fetch the data from the network channel
 * return number of received bytes or error code
 */
int32_t FetchMessage(struct ChannelDesc *channel, char *buf, int32_t count)
{
  int32_t readrest = count;
  int32_t toread = 0;;

  assert(channel != NULL);
  assert(buf != NULL);
  assert(channel->bufpos >= NET_EOF); /* -1 used to mark the end of the data */
  assert(channel->bufpos <= NET_BUFFER_SIZE);
  assert(channel->bufend >= 0);
  assert(channel->bufend <= NET_BUFFER_SIZE);

  /*
   * buffered multi-part messages engine
   * the replacement for the "push-pull" engine. a large multi-part messages
   * and "zero-copy" are used. the "get" channel contain the pointer to zmq buffer
   * and indices (data start, data end)
   * note that the buffer size is a hardcoded constant
   */
  for(readrest = count; readrest > 0; readrest -= toread)
  {

    if(channel->bufpos == NET_EOF) return 0;

    /* calculates available data, if needed update buffer and restart loop */
    toread = MIN(readrest, channel->bufend - channel->bufpos);
    if(toread == 0)
    {
      ReadSocket(channel);
      continue;
    }

    /* there is the data to take */
    memcpy(buf, channel->buffer + channel->bufpos, toread);
    channel->bufpos += toread;
    buf += toread;
  }

  return count - readrest;
}

/*
 * write a multi-part message to the channel
 * return number of sent bytes or error code
 */
static int32_t WriteSocket(struct ChannelDesc *channel, char *buf, int32_t count)
{
  int result;
  int32_t writerest;
  zmq_msg_t msg;
  void *hint = NULL;

  assert(channel != NULL);
  assert(buf != NULL);

  /* emit the message */
  result = zmq_msg_init(&msg);
  ZMQ_TEST_STATE(result, &msg);

  /* send a multi-part message */
  for(writerest = count; writerest > 0; writerest -= NET_BUFFER_SIZE)
  {
    int32_t towrite;
    int32_t flag;

    /* calculate the send mode */
    if(writerest > NET_BUFFER_SIZE)
    {
      towrite = NET_BUFFER_SIZE;
      flag = ZMQ_SNDMORE;
    }
    else
    {
      towrite = writerest;
      flag = 0;
    }

    /* do send */
    result = zmq_msg_init_data(&msg, buf, towrite, ZMQFree, hint);
    ZMQ_TEST_STATE(result, &msg);
    result = zmq_send(channel->socket, &msg, flag);
    ZMQ_TEST_STATE(result, &msg);
    buf += towrite;
  }

  zmq_msg_close(&msg);

  return count;
}

/*
 * send the data to the network channel
 * return number of sent bytes or error code
 */
int32_t SendMessage(struct ChannelDesc *channel, char *buf, int32_t count)
{
  return WriteSocket(channel, buf, count);
}
