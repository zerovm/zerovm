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
#include <stdio.h>
#include "src/main/zlog.h"
#include "src/channels/channel.h"
#include "src/channels/preload.h"
#include "src/channels/prefetch.h"

#define MAX_CHANNELS_NUMBER 10915
#define MIN_CHANNELS_NUMBER 3

static GTree *aliases;
static int tree_reset = 0;

/* TODO(d'b): find better place for the function */
int64_t GetFileSize(const char *name)
{
  struct stat fs;
  int handle = open(name, O_RDONLY);
  return fstat(handle, &fs), close(handle) ? -1 : fs.st_size;
}

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
  int32_t result = -1;

  assert(channel != NULL);

  switch(channel->protocol)
  {
    case ProtoRegular:
      result = pread(GPOINTER_TO_INT(channel->handle), buffer, size, offset);
      if(result == -1)
        result = -errno;
      break;
    case ProtoCharacter:
    case ProtoFIFO:
      result = fread(buffer, 1, size, channel->handle);
      if(result == -1)
        result = -errno;
      break;
    case ProtoOpaque:
      FetchData(channel, buffer, size);
      break;
    default: /* design error */
      ZLOGFAIL(1, EFAULT, "invalid channel source %s", channel->alias);
      break;
  }

  /* update eof status */
  if(result == 0 && CH_SEQ_READABLE(channel))
      channel->eof = 1;

  if(result > 0)
  {
    /* update get position and put position (except CDR) */
    offset += result;
    if(CH_RND_WRITEABLE(channel))
      channel->putpos = offset;
    channel->getpos = offset;

    /* update accounting */
    channel->counters[GetSizeLimit] += result;
  }
  ++channel->counters[GetsLimit];

  return result;
}

int32_t ChannelWrite(struct ChannelDesc *channel,
    const char *buffer, size_t size, off_t offset)
{
  int32_t result = -1;

  switch(channel->protocol)
  {
    case ProtoRegular:
      result = pwrite(GPOINTER_TO_INT(channel->handle), buffer, size, offset);
      break;
    case ProtoCharacter:
    case ProtoFIFO:
      result = fwrite(buffer, 1, size, channel->handle);
      break;
    case ProtoOpaque:
      result = SendData(channel, buffer, size);
      break;
    default: /* design error */
      ZLOGFAIL(1, EFAULT, "invalid channel source %s", channel->alias);
      break;
  }

  /* accounting */
  ZLOGFAIL(result < 0, EIO, "%s failed to write: %s",
      channel->alias, strerror(errno));
  channel->counters[PutSizeLimit] += result;
  ++channel->counters[PutsLimit];

  /* update cursors and size */
  channel->putpos = offset + result;
  channel->size = (channel->type == SGetRPut) || (channel->type == RGetRPut)
      ? MAX(channel->size, channel->putpos) : channel->putpos;
  channel->getpos = channel->putpos;

  return result;
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

/* to sort channels alphabetically by alias */
static int OrderAlias(const struct ChannelDesc **a, const struct ChannelDesc **b)
{
  return g_strcmp0((*a)->alias, (*b)->alias);
}

void SortChannels(GPtrArray *channels)
{
  g_ptr_array_sort(channels, (GCompareFunc)OrderAlias);
  g_ptr_array_sort(channels, (GCompareFunc)OrderUser);
}

/* mount the channel sources */
static void ChannelCtor(struct ChannelDesc *channel)
{
  assert(channel != NULL);

  /* check alias for duplicates and update the list */
  g_tree_insert(aliases, channel->alias, NULL);

  ZLOGFAIL(channel->type > RGetRPut, EFAULT,
      "%s has invalid type %d", channel->alias, channel->type);

  /* mount the channel */
  if(IS_FILE(channel))
    PreloadChannelCtor(channel);
  else
    PrefetchChannelCtor(channel);

  /* mark channel as mounted */
  channel->valid = 1;
}

/* close channel and deallocate its resources */
static void ChannelDtor(struct ChannelDesc *channel)
{
  assert(channel != NULL);

  /* quit if channel isn't mounted */
  if(channel->valid == 0) return;

  /* free channel */
  if(IS_FILE(channel))
    PreloadChannelDtor(channel);
  else
    PrefetchChannelDtor(channel);
}

void ChannelsCtor(struct Manifest *manifest)
{
  int i = 0;

  /* allocate list to detect duplicate channels aliases */
  assert(manifest != NULL);
  assert(aliases == NULL);
  aliases = g_tree_new_full((GCompareDataFunc)strcmp,
      NULL, (GDestroyNotify)DuplicateKey, NULL);

  /*
   * calculate channels count. maximum allowed (MAX_CHANNELS_NUMBER - 1)
   * channels, minimum - RESERVED_CHANNELS
   */
  ZLOGFAIL(manifest->channels->len >= MAX_CHANNELS_NUMBER,
      ENFILE, "channels number reached maximum");
  ZLOGFAIL(manifest->channels->len < MIN_CHANNELS_NUMBER,
      EFAULT, "not enough channels: %d", manifest->channels->len);

  /* mount channels */
  for(i = 0; i < manifest->channels->len; ++i)
    ChannelCtor(CHANNEL(manifest, i));

  /* reorder channels for user manifest */
  SortChannels(manifest->channels);

  /* check if all standard channels are specified */
  ZLOGFAIL(manifest->channels->len <= STDERR_FILENO
      || g_strcmp0(CHANNEL(manifest, STDERR_FILENO)->alias, STDERR),
      EFAULT, "missing standard channels in manifest");
  ResetAliases();
}

void ChannelsDtor(struct Manifest *manifest)
{
  int i;

  /* exit if channels are not constructed */
  if(manifest == NULL || manifest->channels == NULL) return;

  /* close channels */
  for(i = 0; i < manifest->channels->len; ++i)
  {
    struct ChannelDesc *channel = CHANNEL(manifest, i);
    ChannelDtor(channel);
  }
  ResetAliases();
}
