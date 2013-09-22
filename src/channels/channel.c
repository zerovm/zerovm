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
#include "src/main/accounting.h"
#include "src/channels/preload.h"
#include "src/channels/prefetch.h"
#include "src/channels/nservice.h"
#include "src/channels/channel.h"

#define PURE_EVIL 500

/*
 * array of read buffers. WARNING: buffers[0] should not be allocated
 * since it always will point to the user buffer for optimization reason
 */
static GPtrArray *buffers = NULL;
static uint32_t buffers_size = 0; /* size of buffers */
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

/* get chunk of data from source. data will be put to "buffers" */
static int32_t GetDataChunk(struct ChannelDesc *channel, int n,
    size_t size, off_t offset)
{
  int32_t result = 0;

  switch(CH_PROTO(channel, n))
  {
    case ProtoRegular:
      result = pread(GPOINTER_TO_INT(CH_HANDLE(channel, n)),
          buffers->pdata[n], size, offset);
      if(result == -1) result = -errno;
      break;
    case ProtoCharacter:
    case ProtoFIFO:
      result = fread(buffers->pdata[n], 1, size, CH_HANDLE(channel, n));
      if(result == -1) result = -errno;
      break;
    case ProtoTCP:
      /* get another message if it already exhausted */
      if(channel->bufend - channel->bufpos == 0)
        FetchMessage(channel, n);

      /* copy data from the message to buffers */
      if(channel->eof == 0)
      {
        result = MIN(size, channel->bufend - channel->bufpos);
        memcpy(buffers->pdata[n], MessageData(channel) + channel->bufpos, result);
        channel->bufpos += result;
      }
      break;
    default: /* design error */
      ZLOGFAIL(1, EFAULT, "invalid channel source %s;n", channel->alias, n);
      break;
  }

  /* update the source position */
  if(result > 0)
    CH_SOURCE(channel, n)->pos += result;

  if(result == 0)
    if(CH_SEQ_READABLE(channel)) channel->eof = 1;
  return result;
}

/* check EOF digest for network source */
static void TestEOFDigest(struct ChannelDesc *channel, int n)
{
  if(channel->tag != NULL && channel->bufend > 0)
  {
    char digest[TAG_DIGEST_SIZE + 1];
    char *control = MessageData(channel);

    assert(channel->bufend == TAG_DIGEST_SIZE);

    TagDigest(channel->tag, digest);
    if(0 != memcmp(control, digest, TAG_DIGEST_SIZE))
    {
      SetExitState("data corrupted");
      SetExitCode(EPIPE);
      ZLOG(LOG_ERROR, "%s;%d corrupted upon eof", channel->alias, n);
    }
  }
}

/*
 * return the 1st valid source in the raw or -1
 * TODO(d'b): implement a new logic to choose the 1st available source
 */
static int GetFirstSource(struct ChannelDesc *channel)
{
  int n;

  for(n = 0; n < channel->source->len; ++n)
    if(IS_VALID(CH_FLAGS(channel, n))) return n;
  return -1;
}

int32_t ChannelRead(struct ChannelDesc *channel,
    char *buffer, size_t size, off_t offset)
{
  int32_t result = -1;
  int good = -1; /* index of buffer with proper data */
  int readrest = size;
  int toread;
  int n;

  assert(buffers != NULL);
  assert(channel != NULL);
  assert(channel->source->len > 0);

  /* read "size" bytes or until channel EOF */
  while(readrest > 0 && !channel->eof)
  {
    int first = GetFirstSource(channel);
    toread = MIN(readrest, BUFFER_SIZE);
    good = -1;

    ZLOGFAIL(first < 0, EIO, "all %s sources failed", channel->alias);
    for(n = first; n < channel->source->len && good < 0 && !channel->eof; ++n)
    {
      int j;

      /* choose "zero copy" source */
      buffers->pdata[first] = buffer;

      /* get next data portion */
      if(!IS_VALID(CH_FLAGS(channel, n))) continue;
      SyncSource(channel, n);
      result = GetDataChunk(channel, n, toread, offset);
      if(result < 0)
      {
        CH_FLAGS(channel, n) |= FLAG_VALID_MASK;
        continue;
      }

      /* compare buffers */
      for(j = first; j < n; ++j)
      {
        /* skip invalid source buffer */
        if(!IS_VALID(CH_FLAGS(channel, n))) continue;
        if(memcmp(buffers->pdata[j], buffers->pdata[n], result) == 0)
        {
          good = j;
          break;
        }
      }

      /* accounting */
      CountGet(CH_SOURCE(channel, n), result);
    }

    /* fail session if chunk broken and cannot be restored */
    ZLOGFAIL(!channel->eof && good < 0 && channel->source->len > 1,
        EIO, "%s failed to read", channel->alias);
    ZLOGFAIL(result < 0 && channel->source->len == 1,
        EIO, "%s failed to read", channel->alias);

    /* copy verified data to buffer and shift the position */
    if(good > first)
      memcpy(buffer, buffers->pdata[good], result);
    buffer += result;
    offset += result;
    readrest -= result;

    /* update get position and put position (except CDR) */
    if(CH_RND_WRITEABLE(channel)) channel->putpos = offset;
    channel->getpos = offset;
  }

  /* update tag and return actual data size */
  result = size - readrest;
  buffer -= result;
  TagUpdate(channel->tag, buffer, result);

  /* extra corruption check for network source on EOF */
  good = GetFirstSource(channel);
  if(channel->eof && IS_NETWORK(CH_PROTO(channel, good)))
    TestEOFDigest(channel, good);

  /* update user i/o counters */
  ++channel->counters[GetsLimit];
  if(result > 0)
    channel->counters[GetSizeLimit] += result;
  return result;
}

int32_t ChannelWrite(struct ChannelDesc *channel,
    const char *buffer, size_t size, off_t offset)
{
  int n;
  int32_t result = -1;

  ZLOG(LOG_INSANE, "channel %s, buffer=0x%lx, size=%d, offset=%ld",
      channel->alias, (intptr_t)buffer, size, offset);

  for(n = 0; n < channel->source->len; ++n)
  {
    switch(CH_PROTO(channel, n))
    {
      case ProtoRegular:
        result = pwrite(GPOINTER_TO_INT(CH_HANDLE(channel, n)), buffer, size, offset);
        break;
      case ProtoCharacter:
      case ProtoFIFO:
        result = fwrite(buffer, 1, size, CH_HANDLE(channel, n));
        break;
      case ProtoTCP:
        result = SendData(channel, n, buffer, size);
        break;
      default: /* design error */
        ZLOGFAIL(1, EFAULT, "invalid channel source %s;%d", channel->alias, n);
        break;
    }

    /* accounting */
    ZLOGFAIL(result < 0, EIO, "%s;%d failed to write: %s",
        channel->alias, n, strerror(errno));
    CountPut(CH_SOURCE(channel, n), result);
  }

  /* update cursors and size */
  channel->putpos = offset + result;
  channel->size = (channel->type == SGetRPut) || (channel->type == RGetRPut) ?
      MAX(channel->size, channel->putpos) : channel->putpos;
  channel->getpos = channel->putpos;
  TagUpdate(channel->tag, buffer, result);

  /* update user i/o counters */
  ++channel->counters[PutsLimit];
  if(result > 0)
    channel->counters[PutSizeLimit] += result;
  return result;
}

/* get network sources statistics (RO - binds, WO - connects) */
static void CountNetSources(const struct ChannelDesc *channel,
    uint32_t *binds_number, uint32_t *connects_number)
{
  int i;

  for(i = 0; i < channel->source->len; ++i)
  {
    struct Connection *c = CH_SOURCE(channel, i);

    /* update binds/connects statistics */
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

/* order channels to dismounting sequence ("connects" before "binds") */
static int OrderDismount(const struct ChannelDesc **a, const struct ChannelDesc **b)
{
  return OrderMount(b, a);
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

/* to sort sources, network sources before local */
static int OrderSources(const struct Connection **a, const struct Connection **b)
{
  return IS_FILE((*a)->protocol) - IS_FILE((*b)->protocol);
}

/* get numbers of "binds" and "connects" sources */
static void GetNetworkStatistics(const struct Manifest *manifest)
{
  int i;

  for(i = 0; i < manifest->channels->len; ++i)
    CountNetSources(CH_CH(manifest, i), &binds, &connects);
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

  /* sort sources if channel is RO */
  if(IS_RO(channel))
    g_ptr_array_sort(channel->source, (GCompareFunc)OrderSources);

  /* update "buffers" size for "read" channels */
  if(IS_RO(channel) || IS_RW(channel))
    if(channel->source->len > buffers_size)
      buffers_size = channel->source->len;
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

  /*
   * message cannot be safely deallocated until 0mq context closed
   * since message can still be in use
   */
  FreeMessage(channel);
}

void ChannelsCtor(struct Manifest *manifest)
{
  int i = 0;

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
  if(binds + connects > 0)
    NetCtor(manifest);

  /* mount RO channels */
  while(IS_RO(CH_CH(manifest, i)))
    ChannelCtor(CH_CH(manifest, i++));

  /* ask for name service */
  NameServiceCtor(manifest, binds, connects);
  NameServiceDtor();

  /* mount the rest of channels */
  for(; i < manifest->channels->len; ++i)
    ChannelCtor(CH_CH(manifest, i));

  /* reorder channels for user manifest */
  g_ptr_array_sort(manifest->channels, (GCompareFunc)OrderUser);

  /* check if all standard channels are specified */
  ZLOGFAIL(manifest->channels->len <= STDERR_FILENO
      || g_strcmp0(CH_CH(manifest, STDERR_FILENO)->alias, STDERR),
      EFAULT, "missing standard channels in manifest");

  ResetAliases();

  /* allocate read buffers reserving index 0 for "zero copy" */
  buffers = g_ptr_array_new();
  g_ptr_array_add(buffers, NULL);
  for(i = 1; i < buffers_size; ++i)
    g_ptr_array_add(buffers, g_malloc(BUFFER_SIZE));

  /*
   * TODO(d'b): 0mq can drop 1st message if 0mq connection procedure
   * is not complete. this is the temporary fix. should be removed asap
   * because it means zerovm startup latency. NOTE: only need if the
   * session have network channels
   */
  if(binds + connects > 0) usleep(PURE_EVIL);
}

void ChannelsDtor(struct Manifest *manifest)
{
  int i;

  /* exit if channels are not constructed */
  if(manifest == NULL || manifest->channels == NULL) return;

  /* reverse the sort order and close channels */
  g_ptr_array_sort(manifest->channels, (GCompareFunc)OrderDismount);
  for(i = 0; i < manifest->channels->len; ++i)
  {
    struct ChannelDesc *channel = CH_CH(manifest, i);
    ReportTag(channel->alias, channel->tag);
    ChannelDtor(channel);
  }
  ResetAliases();

  /* release read buffers */
  if(buffers != NULL)
    g_ptr_array_free(buffers, TRUE);

  /* release prefetch class */
  if(binds + connects > 0)
    NetDtor(manifest);
}
