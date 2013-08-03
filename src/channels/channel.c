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
#include <glib.h>
#include "src/loader/sel_ldr.h"
#include "src/main/report.h"
#include "src/channels/preload.h"
#include "src/channels/prefetch.h"
#include "src/channels/nservice.h"
#include "src/channels/channel.h"

#define PREPOLL_WAIT 1000 /* 1 millisecond */

GTree *aliases;
static int tree_reset = 0;
static uint32_t binds = 0; /* "bind" sources number */
static uint32_t connects = 0; /* "connect" sources number */

/* if the function called there is duplicate */
static void DuplicateKey(gpointer key)
{
  ZLOGFAIL(tree_reset == 0, EFAULT, "%s is already allocated", key);
}

/* reset aliases tree */
static void ResetAliases()
{
  if(aliases == NULL) return;
  tree_reset = 1; /* we want to remove data from the tree */
  g_tree_destroy(aliases);
  tree_reset = 0; /* set it back to "check mode" */
  aliases = NULL;
}

int32_t ChannelRead(struct ChannelDesc *channel,
    char *buffer, size_t size, off_t offset)
{
  int n;
  int eof = channel->eof; /* store *source* eof state here */
  int32_t result = -1;
  int net_channel = 0; /* will be set if channel has network source(s) */
  int good = -1; /* buffer index (and helper) containing proper data */
  char **b = g_alloca(channel->source->len * sizeof *b);

  assert(channel->eof == 0);

  for(n = 0; n < channel->source->len; ++n)
  {
    int j;

    /*
     * get a new buffer if a good match not found, otherwise reuse the old one
     * for the very 1st buffer given "buffer" should be used because it is
     * most probable that source will not have errors
     */
    if(n > 0)
      b[n] = good < 0 ? g_malloc(size) : b[good == 0 ? 1 : 0];
    else
      b[0] = buffer;

    /* read data from the source to b[i] */
    switch(CH_PROTO(channel, n))
    {
      case ProtoRegular:
        result = pread(GPOINTER_TO_INT(CH_HANDLE(channel, n)), b[n], size, offset);
        if(result == -1) result = -errno;
        break;
      case ProtoCharacter:
      case ProtoFIFO:
        result = fread(b[n], 1, size, CH_HANDLE(channel, n));
        if(result == -1) result = -errno;
        break;
      case ProtoTCP:
        /* todo: FetchMessage doesn't return -1 in case of error. fix it */
        net_channel = 1;
        result = FetchMessage(channel, n, b[n], size);
        if(result == -1) result = -EIO;

        /*
         * channel can have more then one network source and if eof reached
         * FetchMessage() will not try to read. therefore to avoid skipping
         * read the source, channel->eof reset to 0 and actual eof status
         * stored to local variable to be restored later
         */
        ZLOGFAIL(eof > channel->eof, EPIPE, "%s out of sync", channel->alias);
        eof = channel->eof;
        channel->eof = 0;
        break;
      default: /* design error */
        ZLOGFAIL(1, EFAULT, "invalid channel source");
        break;
    }

    /* compare buffers or skip the loop if a good match already found */
    if(good >= 0) continue;
    for(j = 0; j < n; ++j)
      if(memcmp(b[j], b[n], size) == 0)
      {
        good = j;
        break;
      }
  }

  /* restore channel eof */
  channel->eof = eof;

  /* FAIL session if all sources failed */
  ZLOGFAIL(good < 0 && channel->source->len > 1, EIO,
      "%s failed to read", channel->alias);

  /*
   * set eof if 0 bytes has been read. it is safe because
   * 1. if user asked for a 0 bytes control will not reach this code
   * 2. if user asked more then 0 bytes and got 0 that means end of data
   * 3. if quota exceeded user will get an error before an actual read
   */
  if(result == 0) channel->eof = 1;

  /* copy to buffer proper data or skip */
  if(result > 0)
    if(channel->source->len > 1 && good > 0)
      memcpy(buffer, b[good], result);
  TagUpdate(channel->tag, buffer, result);

  /* if eof reached check tagged channel */
  if(eof && channel->tag != NULL && net_channel)
  {
    char digest[TAG_DIGEST_SIZE + 1];

    TagDigest(channel->tag, digest);
    if(0 != memcmp(GetControlDigest(), digest, TAG_DIGEST_SIZE))
    {
      SetExitState("data corrupted");
      SetExitCode(EPIPE);
      ZLOG(LOG_ERROR, "%s %d corrupted upon eof", channel->alias, n);
    }
  }

  /* free resources */
  for(n = 1; n <= good + 1; ++n)
    g_free(b[n]);

  return result;
}

int32_t ChannelWrite(struct ChannelDesc *channel,
    const char *buffer, size_t size, off_t offset)
{
  int n;
  int32_t result = -1;

  for(n = 0; n < channel->source->len; ++n)
  {
    switch(CH_PROTO(channel, n))
    {
      case ProtoRegular:
        result = pwrite(GPOINTER_TO_INT(CH_HANDLE(channel, n)), buffer, size, offset);
        if(result == -1) result = -errno;
        break;
      case ProtoCharacter:
      case ProtoFIFO:
        result = fwrite(buffer, 1, size, CH_HANDLE(channel, n));
        if(result == -1) result = -errno;
        break;
      case ProtoTCP:
        result = SendMessage(channel, n, buffer, size);
        if(result == -1) result = -EIO;
        break;
      default: /* design error */
        ZLOGFAIL(1, EFAULT, "invalid channel source");
        break;
    }
  }

  TagUpdate(channel->tag, buffer, result);
  return result;
}

/* mount the channel sources */
static void ChannelCtor(struct ChannelDesc *channel)
{
  int i;

  assert(channel != NULL);

  /* check alias for duplicates and update the list */
  g_tree_insert(aliases, channel->alias, NULL);

  ZLOGFAIL(channel->type > RGetRPut, EFAULT,
      "%s has invalid type %d", channel->alias, channel->type);

  /* mount given channel sources */
  for(i = 0; i < channel->source->len; ++i)
    if(IS_FILE(CH_PROTO(channel, i)))
      PreloadChannelCtor(channel, i);
    else
      PrefetchChannelCtor(channel, i);
}

/* close channel and deallocate its resources */
static void ChannelDtor(struct ChannelDesc *channel)
{
  int i;

  assert(channel != NULL);

  /* quit if channel isn't mounted (no handles added) */
  if(channel->source->len == 0) return;

  /* free channel */
  for(i = 0; i < channel->source->len; ++i)
    if(IS_FILE(CH_PROTO(channel, i)))
      PreloadChannelDtor(channel, i);
    else
      PrefetchChannelDtor(channel, i);
}

/* get network sources statistics (RO - binds, WO - connects) */
static void CountNetSources(const struct ChannelDesc *channel,
    uint32_t *binds_number, uint32_t *connects_number)
{
  int i;
  for(i = 0; i < channel->source->len; ++i)
  {
    struct Connection *c = CH_SOURCE(channel, i);

    if(IS_NETWORK(c->protocol))
    {
      if(IS_WO(channel)) ++*connects_number;
      else ++*binds_number;
    }
  }
}

/*
 * order channels to mounting sequence
 * compares 2 channels in a distinct way: place "connect" ones after
 * "binds" and channels without network sources at the end
 */
static int OrderMount(const struct ChannelDesc **a, const struct ChannelDesc **b)
{
  uint32_t a_binds = 0;
  uint32_t b_binds = 0;
  uint32_t a_connects = 0;
  uint32_t b_connects = 0;

  CountNetSources(*a, &a_binds, &a_connects);
  CountNetSources(*b, &b_binds, &b_connects);

  /* compare */
  if(a_binds != 0 || b_binds != 0)
    return b_binds - a_binds;
  else
    if(a_connects != 0 || b_connects != 0)
      return b_connects - a_connects;

  return 0; /* local sources does not matter */
}

/*
 * order channels for user manifest
 * 1st 3 channels should be stdin, stdout, stderr
 */
static int OrderUser(const struct ChannelDesc **a, const struct ChannelDesc **b)
{
  if(g_strcmp0((*a)->alias, STDIN) == 0) return -1;
  if(g_strcmp0((*b)->alias, STDIN) == 0) return 1;
  if(g_strcmp0((*a)->alias, STDOUT) == 0) return -1;
  if(g_strcmp0((*b)->alias, STDOUT) == 0) return 1;
  if(g_strcmp0((*a)->alias, STDERR) == 0) return -1;
  if(g_strcmp0((*b)->alias, STDERR) == 0) return 1;

  return 0; /* the order for other channels does not matter */
}

/* get numbers of "binds" and "connects" sources */
static void GetNetworkStatistics(const struct Manifest *manifest)
{
  int i;

  for(i = 0; i < manifest->channels->len; ++i)
    CountNetSources(CH_CH(manifest, i), &binds, &connects);
}

void ChannelsCtor(struct Manifest *manifest)
{
  int i;

  assert(manifest != NULL);

  /* allocate list to detect duplicate channels aliases */
  assert(aliases == NULL);
  aliases = g_tree_new_full((GCompareDataFunc)strcmp,
      NULL, (GDestroyNotify)DuplicateKey, NULL);

  /*
   * calculate channels count. maximum allowed (MAX_CHANNELS_NUMBER - 1)
   * channels, minimum - RESERVED_CHANNELS
   */
  ZLOGFAIL(manifest->channels->len >= MAX_CHANNELS_NUMBER,
      ENFILE, "channels number reached maximum");

  /* sort channels. then count "binds" / "connects" number */
  g_ptr_array_sort(manifest->channels, (GCompareFunc)OrderMount);
  GetNetworkStatistics(manifest);

  /* construct prefetch class before usage */
  NetCtor(manifest);

  /* mount "binds" number of channels from 0 index */
  for(i = 0; i < binds; ++i)
    ChannelCtor(CH_CH(manifest, i));

  /* ask for name service */
  NameServiceCtor(manifest, binds, connects);
  NameServiceDtor();

  /* mount the rest of channels ("connect" ones) */
  for(i = binds; i < manifest->channels->len; ++i)
    ChannelCtor(CH_CH(manifest, i));

  /* reorder channels for user manifest */
  g_ptr_array_sort(manifest->channels, (GCompareFunc)OrderUser);

  /* check if all standard channels are specified */
  ZLOGFAIL(g_strcmp0(CH_CH(manifest, STDERR_FILENO)->alias, STDERR),
      EFAULT, "missing standard channels in manifest");

  ResetAliases();

  /*
   * temporary fix (the lost 1st message issue) to allow 0mq to complete
   * the connection procedure. 1 millisecond should be enough
   * todo(d'b): replace it with the channels readiness check
   */
  usleep(PREPOLL_WAIT);
}

void ChannelsDtor(struct Manifest *manifest)
{
  int i;

  /* exit if channels are not constructed */
  if(manifest == NULL || manifest->channels == NULL) return;

  for(i = 0; i < manifest->channels->len; ++i)
  {
    struct ChannelDesc *channel = CH_CH(manifest, i);
    ReportTag(channel->alias, channel->tag);
    ChannelDtor(channel);
  }
  ResetAliases();

  /* release prefetch class */
  NetDtor(manifest);
}
