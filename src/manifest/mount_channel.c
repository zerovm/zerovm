/*
 * channels constructor / destructor
 * todo(NETWORKING): update this code with net channels routines
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>

#include "api/zvm.h"
#include "src/utils/tools.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/manifest/manifest_setup.h"
#include "src/manifest/manifest_parser.h"
#include "src/service_runtime/include/sys/errno.h"
#include "src/manifest/preload.h"
#include "src/manifest/premap.h"
#include "src/manifest/prefetch.h"
#include "src/manifest/mount_channel.h"

/*
 * return source type infered from the channel name
 * todo(d'b): rewrite it. so far it assumes local files
 */
#define NET_PREFIX "ipc://"
static int GetSourceType(char *name)
{
  /* check for design errors */
  assert(name != NULL);
  assert(*name != 0);

  return strncmp(name, NET_PREFIX, strlen(NET_PREFIX)) == 0 ?
      NetworkChannel : LocalFile ;
}

/*
 * mount given channel (must be constructed)
 * todo(NETWORKING): update with network channel initialization
 */
static void MountChannel(struct NaClApp *nap, struct ChannelDesc *channel)
{
  assert(nap != NULL);
  assert(channel != NULL);

  switch(GetSourceType((char*)channel->name))
  {
    int code;
    case LocalFile:
      code = PreloadChannelCtor(channel);
      COND_ABORT(code, "cannot allocate local file channel");
      break;
    case NetworkChannel:
      code = PrefetchChannelCtor(channel);
      COND_ABORT(code, "cannot allocate network channel");
      break;
    default:
      NaClLog(LOG_FATAL, "invalid channel source type\n");
      break;
  }
}

/* return the channel by channel type */
static struct ChannelDesc* SelectNextChannel(struct NaClApp *nap, char *type)
{
  static int current_channel = RESERVED_CHANNELS;
  struct ChannelDesc *channel;
  int access_type; /* stdin, stdout, cdr,.. */

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->system_manifest->channels != NULL);
  assert(type != NULL);

#define STD_INIT(ch, num) do {\
    channel = &nap->system_manifest->channels[num];\
    COND_ABORT(STREQ((char*)channel->name, ch), ch " already allocated");\
    channel->type = SGetSPut; /* ### remove it? */\
  } while(0)

  access_type = ATOI(type);
  switch(access_type)
  {
    case Stdin:
      STD_INIT(STDIN, STDIN_FILENO);
    /* ### set and check other attributes */
      break;
    case Stdout:
      STD_INIT(STDOUT, STDOUT_FILENO);
      /* ### set and check other attributes */
      break;
    case Stderr:
      STD_INIT(STDERR, STDERR_FILENO);
      /* ### set and check other attributes */
      break;
    case SGetSPut:
    case RGetSPut:
    case SGetRPut:
    case RGetRPut:
      /* take current channel index */
      channel = &nap->system_manifest->channels[current_channel++];
      channel->type = access_type;
      break;
    default:
      NaClLog(LOG_FATAL, "invalid channel access type\n");
      break;
  }
#undef STD_INIT

  return channel;
}

///* finalize and close InputChannel */
//static void InputChannelDtor(struct NaClApp *nap)
//{
//  struct ChannelDesc *channel;
//
//  assert(nap != NULL);
//  assert(nap->system_manifest != NULL);
//  assert((void*)nap->system_manifest->channels != NULL);
//
//  channel = &((struct ChannelDesc*)nap->system_manifest->channels)[InputChannel];
//  if(channel->name) close(channel->handle);
//}
//
///* finalize and close OutputChannel */
//static void OutputChannelDtor(struct NaClApp *nap)
//{
//  struct ChannelDesc *channel;
//
//  assert(nap != NULL);
//  assert(nap->system_manifest != NULL);
//  assert((void*)nap->system_manifest->channels != NULL);
//
//  /* todo: find the way how to determine file size for "premapped" output */
//  channel = &((struct ChannelDesc*)nap->system_manifest->channels)[OutputChannel];
//  if(channel->name) close(channel->handle);
//}
//
///* finalize and close LogChannel */
//static void LogChannelDtor(struct NaClApp *nap)
//{
//  struct ChannelDesc* channel;
//
//  assert(nap != NULL);
//  assert(nap->system_manifest != NULL);
//  assert((void*)nap->system_manifest->channels != NULL);
//
//  /*
//   * trim user log and close the channel
//   * todo(d'b): find a solution (for binary content) to avoid truncate()
//   */
//  channel = &((struct ChannelDesc*)nap->system_manifest->channels)[LogChannel];
//
//  if (channel->name && channel->buffer)
//  {
//    char *p = (char*) NaClUserToSys(nap, (uint32_t)channel->buffer);
//    channel->fsize = strlen(p);
//    munmap(p, channel->bsize);
//    if(channel->fsize) truncate((char*)channel->name, channel->fsize);
//    else remove((char*)channel->name);
//  }
//}
//
///*
// * finalize and close NetworkInputChannel
// * todo(NETWORKING): put NetworkInputChannel finalizer here
// * note: there can be more than one NetworkInputChannel channels
// */
//static void NetworkInputChannelDtor(struct NaClApp *nap, enum ChannelType ch)
//{
//  struct ChannelDesc *channel;
//
//  assert(nap != NULL);
//  assert(nap->system_manifest != NULL);
//  assert((void*)nap->system_manifest->channels != NULL);
//
//  UNREFERENCED_PARAMETER(channel);
//}
//
///*
// * finalize and close NetworkOutputChannel
// * todo(NETWORKING): put NetworkOutputChannel finalizer here
// * note: there can be more than one NetworkOutputChannel channels
// */
//static void NetworkOutputChannelDtor(struct NaClApp *nap, enum ChannelType ch)
//{
//  struct ChannelDesc *channel;
//
//  assert(nap != NULL);
//  assert(nap->system_manifest != NULL);
//  assert((void*)nap->system_manifest->channels != NULL);
//
//  UNREFERENCED_PARAMETER(channel);
//}

/* construct and initialize the channel */
static void ChannelCtor(struct NaClApp *nap, char **tokens)
{
  struct ChannelDesc *channel;

  assert(nap != NULL);
  assert(tokens != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->system_manifest->channels != NULL);

  /* pick the channel. "access type" attribute will be set inside */
  channel = SelectNextChannel(nap, tokens[ChannelAccessType]);
  channel->name = tokens[ChannelName];
  channel->handle = ATOI(tokens[ChannelId]);

  /* limits and counters */
  channel->limits[GetsLimit] = ATOI(tokens[ChannelGets]);
  channel->limits[GetSizeLimit] = ATOI(tokens[ChannelGetSize]);
  channel->limits[PutsLimit] = ATOI(tokens[ChannelPuts]);
  channel->limits[PutSizeLimit] = ATOI(tokens[ChannelPutSize]);
  channel->counters[GetsLimit] = 0;
  channel->counters[GetSizeLimit] = 0;
  channel->counters[PutsLimit] = 0;
  channel->counters[PutSizeLimit] = 0;

  /* initialize the channel */
  MountChannel(nap, channel);
}

/* close channel and deallocate its resources */
static void ChannelDtor(struct ChannelDesc *channel)
{
  assert(channel != NULL);

  switch(GetSourceType((char*)channel->name))
  {
    case LocalFile:
      PreloadChannelDtor(channel);
      break;
    case NetworkChannel:
      PrefetchChannelDtor(channel);
      break;
    default:
      NaClLog(LOG_ERROR, "unknown channel source\n");
      break;
  }
}

/*
 * construct and initialize all channels
 * note: standard channels must be specified in manifest
 */
void ChannelsCtor(struct NaClApp *nap)
{
  int i;
  char *values[MAX_CHANNELS_NUMBER];
  struct SystemManifest *mft;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /* calculate channels count. allowed (MAX_CHANNELS_NUMBER - 1) channels */
  mft = nap->system_manifest;
  mft->channels_count =
      GetValuesByKey("Channel", values, MAX_CHANNELS_NUMBER);
  COND_ABORT(mft->channels_count >= MAX_CHANNELS_NUMBER,
      "channels number reached maximum");

  /* exit if no channels requested */
  if(mft->channels_count == 0) return;

  /* allocate memory for channels */
  mft->channels =
      malloc(sizeof(*mft->channels) * (mft->channels_count + RESERVED_CHANNELS));
  COND_ABORT(mft->channels == NULL, "cannot allocate memory for channels");

  /* parse channels. 0..2 reserved for stdin/stdout/stderr */
  for(i = 0; i < mft->channels_count; ++i)
  {
    char *tokens[CHANNEL_ATTRIBUTES + 1]; // to detect wrong attributes number
    int count = ParseValue(values[i], ", \t", tokens, CHANNEL_ATTRIBUTES + 1);
    COND_ABORT(count != CHANNEL_ATTRIBUTES, "wrong number of the channel attributes");

    /* construct and initialize channel */
    ChannelCtor(nap, tokens);
  }

  /* channels array validation */
  for(i = 0; i < nap->system_manifest->channels_count; ++i)
    COND_ABORT(nap->system_manifest->channels[i].name == (int64_t)NULL,
               "the channels array must not have uninitialized elements");
  COND_ABORT(nap->system_manifest->channels_count < RESERVED_CHANNELS,
      "some of the standard channels weren't initialized");
}

/* finalize all channels and free memory */
void ChannelsDtor(struct NaClApp *nap)
{
  int i;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->system_manifest->channels != NULL);
  assert(nap->system_manifest->channels_count > 0);

  for(i = 0; i < nap->system_manifest->channels_count; ++i)
    ChannelDtor(&nap->system_manifest->channels[i]);

  free(nap->system_manifest->channels);
}
