/*
 * channels constructor / destructor
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */
#include <assert.h>
#include "src/service_runtime/etag.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/manifest/manifest_setup.h"
#include "src/manifest/manifest_parser.h"
#include "src/manifest/preload.h"
#include "src/manifest/prefetch.h"
#include "src/manifest/mount_channel.h"

/* get string contain protocol name by channel source type */
#define SOURCE_PREFIX(type) case type: return prefix[type]
char *StringizeChannelSourceType(enum ChannelSourceType type)
{
  char *prefix[] = CHANNEL_SOURCE_PREFIXES;

  switch(type)
  {
    SOURCE_PREFIX(ChannelRegular);
    SOURCE_PREFIX(ChannelDirectory);
    SOURCE_PREFIX(ChannelCharacter);
    SOURCE_PREFIX(ChannelBlock);
    SOURCE_PREFIX(ChannelFIFO);
    SOURCE_PREFIX(ChannelLink);
    SOURCE_PREFIX(ChannelSocket);
    SOURCE_PREFIX(ChannelIPC);
    SOURCE_PREFIX(ChannelTCP);
    SOURCE_PREFIX(ChannelINPROC);
    SOURCE_PREFIX(ChannelPGM);
    SOURCE_PREFIX(ChannelEPGM);
    SOURCE_PREFIX(ChannelUDP);
    case ChannelSourceTypeNumber: break; /* prevent warning */
  }
  return NULL;
}
#undef SOURCE_PREFIX

/* return source type inferred from the source file (or url) */
static enum ChannelSourceType GetSourceType(char *name)
{
  enum ChannelSourceType type;

  assert(name != NULL);

  /* network channel always contain ':'s */
  if(strchr(name, ':') == NULL)
    type = GetChannelSource(name); // ###
  else
    type = GetChannelProtocol(name);

  COND_ABORT(type == ChannelSourceTypeNumber, "protocol isn't supported");
  return type;
}

/* return the channel by channel type */
static struct ChannelDesc* SelectNextChannel(struct NaClApp *nap, char *alias)
{
  static int current_channel = RESERVED_CHANNELS;
  struct ChannelDesc *channels;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->system_manifest->channels != NULL);
  assert(alias != NULL);

  channels = nap->system_manifest->channels;

  /* check for the standard names */
  if(STREQ(alias, STDIN)) return &channels[STDIN_FILENO];
  if(STREQ(alias, STDOUT)) return &channels[STDOUT_FILENO];
  if(STREQ(alias, STDERR)) return &channels[STDERR_FILENO];

  /* otherwise just return next channel */
  return &channels[current_channel++];
}

/* construct and initialize the channel */
static void ChannelCtor(struct NaClApp *nap, char **tokens)
{
  struct ChannelDesc *channel;

  assert(nap != NULL);
  assert(tokens != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->system_manifest->channels != NULL);

  /*
   * pick the channel and check if the channel is available
   * channels must not have duplicate aliases
   */
  channel = SelectNextChannel(nap, tokens[ChannelAlias]);
  COND_ABORT(channel->alias != NULL, "channel is already allocated");

  /* set common general fields */
  channel->type = ATOI(tokens[ChannelAccessType]);
  COND_ABORT(channel->type < SGetSPut || channel->type > RGetRPut,
      "invalid channel access type");
  channel->name = tokens[ChannelName];
  channel->alias = tokens[ChannelAlias];
  channel->source = GetSourceType((char*)channel->name);

  if(TagEngineEnabled())
  {
    COND_ABORT(TagCtor(&channel->tag) == ERR_CODE, "channel tag setup error");
    memset(channel->digest, 0, TAG_DIGEST_SIZE);
  }

  /* limits and counters */
  channel->limits[GetsLimit] = ATOI(tokens[ChannelGets]);
  channel->limits[GetSizeLimit] = ATOI(tokens[ChannelGetSize]);
  channel->limits[PutsLimit] = ATOI(tokens[ChannelPuts]);
  channel->limits[PutSizeLimit] = ATOI(tokens[ChannelPutSize]);
  channel->counters[GetsLimit] = 0;
  channel->counters[GetSizeLimit] = 0;
  channel->counters[PutsLimit] = 0;
  channel->counters[PutSizeLimit] = 0;
  channel->eof = 0;

  /* mount given channel */
  switch(channel->source)
  {
    int code;
    case ChannelRegular:
    case ChannelCharacter:
      code = PreloadChannelCtor(channel);
      COND_ABORT(code, "cannot allocate local file channel");
      break;
    case ChannelTCP:
      code = PrefetchChannelCtor(channel);
      COND_ABORT(code, "cannot allocate network channel");
      break;
    case ChannelDirectory:
    case ChannelBlock:
    case ChannelFIFO:
    case ChannelLink:
    case ChannelSocket:
    case ChannelIPC:
    case ChannelINPROC:
    case ChannelPGM:
    case ChannelEPGM:
    case ChannelUDP:
      NaClLog(LOG_FATAL, "'%s' isn't supported",
          StringizeChannelSourceType(channel->source));
      break;
    default:
      NaClLog(LOG_FATAL, "invalid channel source. internal error!");
      break;
  }
}

/* close channel and deallocate its resources */
static void ChannelDtor(struct ChannelDesc *channel)
{
  assert(channel != NULL);

  /* set the tag digests for each channel */
  if(TagEngineEnabled())
  {
    /* if not already set */
    if(channel->digest[0] == 0)
      TagDigest(channel->tag, channel->digest);

    NaClLog(LOG_DEBUG, "channel %s closed with etag = %s, getsize = %ld, "
        "putsize = %ld", channel->alias, channel->digest,
        channel->counters[GetSizeLimit], channel->counters[PutSizeLimit]);
  }

  switch(channel->source)
  {
    case ChannelRegular:
    case ChannelCharacter:
      PreloadChannelDtor(channel);
      break;
    case ChannelTCP:
      PrefetchChannelDtor(channel);
      break;
    case ChannelDirectory:
    case ChannelBlock:
    case ChannelFIFO:
    case ChannelLink:
    case ChannelSocket:
    case ChannelIPC:
    case ChannelINPROC:
    case ChannelPGM:
    case ChannelEPGM:
    case ChannelUDP:
      NaClLog(LOG_FATAL, "'%s' isn't supported. internal error!",
          StringizeChannelSourceType(channel->source));
      break;
    default:
      NaClLog(LOG_FATAL, "invalid channel source. internal error!");
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

  /*
   * calculate channels count. maximum allowed (MAX_CHANNELS_NUMBER - 1)
   * channels, minimum - RESERVED_CHANNELS
   */
  mft = nap->system_manifest;
  mft->channels_count =
      GetValuesByKey("Channel", values, MAX_CHANNELS_NUMBER);
  COND_ABORT(mft->channels_count >= MAX_CHANNELS_NUMBER,
      "channels number reached maximum");
  COND_ABORT(mft->channels_count < RESERVED_CHANNELS,
      "not all standard channels are provided");

  /* allocate memory for channels */
  mft->channels = calloc(mft->channels_count, sizeof(*mft->channels));
  COND_ABORT(mft->channels == NULL, "cannot allocate memory for channels");

  /* parse channels. 0..2 reserved for stdin/stdout/stderr */
  for(i = 0; i < mft->channels_count; ++i)
  {
    char *tokens[CHANNEL_ATTRIBUTES + 1]; /* to detect wrong attributes number */
    int count = ParseValue(values[i], ", \t", tokens, CHANNEL_ATTRIBUTES + 1);
    COND_ABORT(count != CHANNEL_ATTRIBUTES, "wrong number of the channel attributes");

    /* construct and initialize channel */
    ChannelCtor(nap, tokens);
  }

  /* channels array validation */
  for(i = 0; i < nap->system_manifest->channels_count; ++i)
    COND_ABORT(nap->system_manifest->channels[i].name == NULL,
               "the channels array must not have uninitialized elements");
  COND_ABORT(nap->system_manifest->channels_count < RESERVED_CHANNELS,
      "some of the standard channels weren't initialized");

  /* 2nd pass for the network channels if name service specified */
  KickPrefetchChannels(nap);
}

/* finalize all channels and free memory */
void ChannelsDtor(struct NaClApp *nap)
{
  int i;

  /* exit if channels are not constructed */
  assert(nap != NULL);
  if(nap->system_manifest == NULL) return;
  if(nap->system_manifest->channels == NULL) return;
  if(nap->system_manifest->channels_count == 0) return;

  for(i = 0; i < nap->system_manifest->channels_count; ++i)
    ChannelDtor(&nap->system_manifest->channels[i]);
  free(nap->system_manifest->channels);
}
