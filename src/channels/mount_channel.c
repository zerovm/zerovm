/*
 * channels constructor / destructor
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
#include <glib.h>
#include "src/main/etag.h"
#include "src/loader/sel_ldr.h"
#include "src/main/manifest_setup.h"
#include "src/main/manifest_parser.h"
#include "src/channels/preload.h"
#include "src/channels/prefetch.h"
#include "src/channels/mount_channel.h"

char *StringizeChannelSourceType(enum ChannelSourceType type)
{
  char *prefix[] = CHANNEL_SOURCE_PREFIXES;

  assert(type >= ChannelRegular && type < ChannelSourceTypeNumber);
  return prefix[type];
}

/* return source type inferred from the source file (or url) */
static enum ChannelSourceType GetSourceType(char *name)
{
  enum ChannelSourceType type;

  assert(name != NULL);

  /* unlike local network channels always contain ':'s */
  if(strchr(name, ':') == NULL)
    type = GetChannelSource(name);
  else
    type = GetChannelProtocol(name);

  ZLOGFAIL(type == ChannelSourceTypeNumber, EPROTONOSUPPORT,
      "can't detect source of %s", name);
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
  int code = 1; /* means error */

  assert(nap != NULL);
  assert(tokens != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->system_manifest->channels != NULL);

  /*
   * pick the channel and check if the channel is available
   * channels must not have duplicate aliases
   */
  channel = SelectNextChannel(nap, tokens[ChannelAlias]);
  ZLOGFAIL(channel->alias != NULL, EFAULT, "%s is already allocated", channel->alias);

  /* set common general fields */
  channel->type = ATOI(tokens[ChannelAccessType]);
  ZLOGFAIL(channel->type < SGetSPut || channel->type > RGetRPut,
      EFAULT, "invalid channel access type %d", channel->type);
  channel->name = tokens[ChannelName];
  channel->alias = tokens[ChannelAlias];
  channel->source = GetSourceType((char*)channel->name);

  /* initialize the channel tag */
  if(CHANNELS_ETAG_ENABLED)
  {
    channel->tag = TagCtor();
    memset(channel->digest, 0, TAG_DIGEST_SIZE);
    memset(channel->control, 0, TAG_DIGEST_SIZE);
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
    case ChannelRegular:
    case ChannelCharacter:
    case ChannelFIFO:
    case ChannelSocket:
      code = PreloadChannelCtor(channel);
      break;
    case ChannelTCP:
      code = PrefetchChannelCtor(channel);
      break;
    case ChannelDirectory:
    case ChannelBlock:
    case ChannelLink:
    case ChannelIPC:
    case ChannelINPROC:
    case ChannelPGM:
    case ChannelEPGM:
    case ChannelUDP:
    default:
      ZLOGFAIL(1, EPROTONOSUPPORT, "'%s': '%s' type isn't supported",
          channel->name, StringizeChannelSourceType(channel->source));
      break;
  }
  ZLOGFAIL(code, EFAULT, "cannot allocate channel %s", channel->alias);
}

/* close channel and deallocate its resources */
static void ChannelDtor(struct ChannelDesc *channel)
{
  assert(channel != NULL);

  /* quit if channel isn't mounted */
  if(channel->name == NULL) return;

  switch(channel->source)
  {
    case ChannelRegular:
    case ChannelCharacter:
    case ChannelFIFO:
    case ChannelSocket:
      PreloadChannelDtor(channel);
      break;
    case ChannelTCP:
      /*
       * since there is no chance to finalize network channels
       * in case if session crashed, the channel destructor just
       * skips it
       */
      if(GetExitCode() == 0)
        PrefetchChannelDtor(channel);
      break;
    case ChannelDirectory:
    case ChannelBlock:
    case ChannelLink:
    case ChannelIPC:
    case ChannelINPROC:
    case ChannelPGM:
    case ChannelEPGM:
    case ChannelUDP:
    default:
      ZLOG(LOG_ERR, "'%s': '%s' type isn't supported",
          channel->name, StringizeChannelSourceType(channel->source));
      break;
  }
}

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
      GetValuesByKey(MFT_CHANNEL, values, MAX_CHANNELS_NUMBER);
  ZLOGFAIL(mft->channels_count >= MAX_CHANNELS_NUMBER, ENFILE,
      "channels number reached maximum");
  ZLOGFAIL(mft->channels_count < RESERVED_CHANNELS, EFAULT,
      "not all standard channels are provided");

  /* allocate memory for channels */
  mft->channels = g_malloc0(mft->channels_count * sizeof(*mft->channels));

  /* parse channels. 0..2 reserved for stdin/stdout/stderr */
  for(i = 0; i < mft->channels_count; ++i)
  {
    char *tokens[CHANNEL_ATTRIBUTES + 1]; /* to detect wrong attributes number */
    int count = ParseValue(values[i], ", \t", tokens, CHANNEL_ATTRIBUTES + 1);
    ZLOGFAIL(count != CHANNEL_ATTRIBUTES, EFAULT, "invalid specification '%s'", values[i]);

    /* construct and initialize channel */
    ChannelCtor(nap, tokens);
  }

  /* channels array validation */
  for(i = 0; i < nap->system_manifest->channels_count; ++i)
    ZLOGFAIL(nap->system_manifest->channels[i].name == NULL, EFAULT,
        "the channels array must not have uninitialized elements");

  ZLOGFAIL(nap->system_manifest->channels_count < RESERVED_CHANNELS, EFAULT,
      "there were uninitialized standard channels");

  /* 2nd pass for the network channels if name service specified */
  KickPrefetchChannels(nap);
}

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
  g_free(nap->system_manifest->channels);
}
