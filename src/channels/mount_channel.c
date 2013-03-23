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

  ZLOGFAIL(type == ChannelSourceTypeNumber,
      EPROTONOSUPPORT, "cannot detect source of %s", name);
  return type;
}

/* return the channel index by channel alias */
static int SelectNextChannel(struct NaClApp *nap, char *alias)
{
  static int current_channel = RESERVED_CHANNELS;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->system_manifest->channels != NULL);
  assert(alias != NULL);

  if(STREQ(alias, STDIN)) return STDIN_FILENO;
  if(STREQ(alias, STDOUT)) return STDOUT_FILENO;
  if(STREQ(alias, STDERR)) return STDERR_FILENO;
  return current_channel++;
}

/* construct and initialize the channel */
static void ChannelCtor(struct NaClApp *nap, char **tokens)
{
  struct ChannelDesc *channel;
  int code = 1; /* means error */
  int index;

  assert(nap != NULL);
  assert(tokens != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->system_manifest->channels != NULL);

  /*
   * pick the channel and check if the channel is available,
   * then allocate space to store the channel information
   */
  index = SelectNextChannel(nap, tokens[ChannelAlias]);
  ZLOGFAIL(index >= nap->system_manifest->channels_count,
      EFAULT, "uninitialized standard channels detected");
  channel = &nap->system_manifest->channels[index];
  ZLOGFAIL(channel->mounted == MOUNTED, EFAULT,
      "%s is already allocated", tokens[ChannelAlias]);

  /* set common general fields */
  channel->type = ATOI(tokens[ChannelAccessType]);
  ZLOGFAIL(channel->type < SGetSPut || channel->type > RGetRPut,
      EFAULT, "invalid access type for %s", tokens[ChannelAlias]);
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

  /* limits and counters. initialize all field explicitly */
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
      ZLOGFAIL(1, EPROTONOSUPPORT, "%s has invalid type %s",
          channel->alias, StringizeChannelSourceType(channel->source));
      break;
  }
  ZLOGFAIL(code, EFAULT, "cannot allocate %s", channel->alias);
  channel->mounted = MOUNTED;
}

/* close channel and deallocate its resources */
static void ChannelDtor(struct ChannelDesc *channel)
{
  assert(channel != NULL);

  /* quit if channel isn't mounted */
  if(channel->mounted != MOUNTED) return;

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
       * since there is a chance to hang up upon the network channels
       * finalization in case if session crashed, the channel destructor
       * just skips it
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
      ZLOG(LOG_ERR, "%s has invalid type %s",
          channel->alias, StringizeChannelSourceType(channel->source));
      break;
  }
  channel->mounted = !MOUNTED;
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
  ZLOGFAIL(mft->channels_count >= MAX_CHANNELS_NUMBER,
      ENFILE, "channels number reached maximum");
  ZLOGFAIL(mft->channels_count < RESERVED_CHANNELS,
      EFAULT, "not all standard channels are provided");

  /* allocate memory for channels pointers */
  mft->channels = g_malloc0(mft->channels_count * sizeof *mft->channels);

  /* parse and mount channels */
  for(i = 0; i < mft->channels_count; ++i)
  {
    char *tokens[CHANNEL_ATTRIBUTES + 1];
    int count = ParseValue(values[i], ",", tokens, CHANNEL_ATTRIBUTES + 1);

    /* fail if invalid number of attributes detected */
    ZLOGFAIL(count != CHANNEL_ATTRIBUTES, EFAULT,
        "invalid channel specification: %s", values[i]);

    /* construct and initialize channel */
    ChannelCtor(nap, tokens);
  }

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
