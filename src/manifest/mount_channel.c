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

static char *prefixes[] = CHANNEL_PREFIXES;
#define PREFIX(ch) prefixes[ch] /* get channel name by channel id */

/*
 * mount given channel (must be constructed)
 * todo(NETWORKING): update with network channel initialization
 */
static void MountChannel(struct NaClApp *nap, enum ChannelType ch)
{
  struct ChannelDesc *channel;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert(ch >= InputChannel && ch < nap->system_manifest->channels_count);
  assert((void*)nap->system_manifest->channels != NULL);

  channel = &((struct ChannelDesc*)nap->system_manifest->channels)[ch];

  if(channel)
  {
    switch(channel->mounted)
    {
      int code;
      case MAPPED:
        code = PremapChannel(nap, channel);
        COND_ABORT(code, "cannot premap channel");
        break;
      case LOADED:
        code = PreloadChannel(nap, channel);
        COND_ABORT(code, "cannot preload channel");
        break;
      case NETWORK:
        code = PrefetchChannel(nap, channel);
        COND_ABORT(code, "cannot allocate network channel");
        break;
      default:
        COND_ABORT(1, "mounting method is not supported");
        break;
    }
  }
}

/* finalize and close InputChannel */
static void InputChannelDtor(struct NaClApp *nap)
{
  struct ChannelDesc *channel;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert((void*)nap->system_manifest->channels != NULL);

  channel = &((struct ChannelDesc*)nap->system_manifest->channels)[InputChannel];
  if(channel->name) close(channel->handle);
}

/* finalize and close OutputChannel */
static void OutputChannelDtor(struct NaClApp *nap)
{
  struct ChannelDesc *channel;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert((void*)nap->system_manifest->channels != NULL);

  /* todo: find the way how to determine file size for "premapped" output */
  channel = &((struct ChannelDesc*)nap->system_manifest->channels)[OutputChannel];
  if(channel->name) close(channel->handle);
}

/* finalize and close LogChannel */
static void LogChannelDtor(struct NaClApp *nap)
{
  struct ChannelDesc* channel;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert((void*)nap->system_manifest->channels != NULL);

  /*
   * trim user log and close the channel
   * todo(d'b): find a solution (for binary content) to avoid truncate()
   */
  channel = &((struct ChannelDesc*)nap->system_manifest->channels)[LogChannel];

  if (channel->name && channel->buffer)
  {
    char *p = (char*) NaClUserToSys(nap, (uint32_t)channel->buffer);
    channel->fsize = strlen(p);
    munmap(p, channel->bsize);
    if(channel->fsize) truncate((char*)channel->name, channel->fsize);
    else remove((char*)channel->name);
  }
}

/*
 * finalize and close NetworkInputChannel
 * todo(NETWORKING): put NetworkInputChannel finalizer here
 * note: there can be more than one NetworkInputChannel channels
 */
static void NetworkInputChannelDtor(struct NaClApp *nap, enum ChannelType ch)
{
  struct ChannelDesc *channel;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert((void*)nap->system_manifest->channels != NULL);

  UNREFERENCED_PARAMETER(channel);
}

/*
 * finalize and close NetworkOutputChannel
 * todo(NETWORKING): put NetworkOutputChannel finalizer here
 * note: there can be more than one NetworkOutputChannel channels
 */
static void NetworkOutputChannelDtor(struct NaClApp *nap, enum ChannelType ch)
{
  struct ChannelDesc *channel;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert((void*)nap->system_manifest->channels != NULL);

  UNREFERENCED_PARAMETER(channel);
}

/*
 * construct channel, mount it and update system_manifest.
 * note: space for channels must be already allocated.
 * todo(NETWORKING): put network channel initialization here
 * if successful return 0
 */
void ChannelCtor(struct NaClApp *nap, enum ChannelType ch)
{
  struct ChannelDesc *channel;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert(ch >= InputChannel && ch < nap->system_manifest->channels_count);
  assert((void*)nap->system_manifest->channels != NULL);

  /*
   * if channel is not set in manifest and the name will be NULL and
   * that fact will be used later to determine which channels are available
   */
  channel = &((struct ChannelDesc*)nap->system_manifest->channels)[ch];
  channel->name = (uint64_t)GetValueByKey(PREFIX(ch));
  if(channel->name)
  {
#define SET_LIMIT(val, suffix) \
  do {\
    char str[KEYWORD_SIZE_MAX];\
    snprintf(str, KEYWORD_SIZE_MAX, "%s%s", PREFIX(ch), suffix);\
    GET_INT_BY_KEY(val, str);\
  } while (0);

    SET_LIMIT(channel->mounted, "Mode");
    channel->self_size = sizeof(*channel);
    channel->type = ch;

    /* set limits */
    SET_LIMIT(channel->limits[GetSizeLimit], "MaxGet");
    SET_LIMIT(channel->limits[GetsLimit], "MaxGetCnt");
    SET_LIMIT(channel->limits[PutSizeLimit], "MaxPut");
    SET_LIMIT(channel->limits[PutsLimit], "MaxPutCnt");

#undef SET_LIMIT

    /* set counters */
    channel->counters[GetSizeCounter] = 0;
    channel->counters[GetsCounter] = 0;
    channel->counters[PutSizeCounter] = 0;
    channel->counters[PutsCounter] = 0;

    MountChannel(nap, ch);
  }
}

/*
 * close / deallocate resources used by channel
 * todo(NETWORKING): put net channels finalizers here
 */
void ChannelDtor(struct NaClApp *nap, enum ChannelType ch)
{
  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert(ch >= InputChannel && ch < nap->system_manifest->channels_count);
  assert((void*)nap->system_manifest->channels != NULL);

  /* by channel number choose finalization routine */
  switch(ch)
  {
    case InputChannel:
      InputChannelDtor(nap);
      break;
    case OutputChannel:
      OutputChannelDtor(nap);
      break;
    case LogChannel:
      LogChannelDtor(nap);
      break;
    default: /* network channels */
      if(0)
      {
        NetworkInputChannelDtor(nap, ch);
        NetworkOutputChannelDtor(nap, ch);
      }
      break;
  }
}
