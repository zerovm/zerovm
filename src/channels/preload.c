/*
 * preload given file to channel.
 * note: with this class we make paging engine
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
#include <sys/fcntl.h>
#include <assert.h>
#include "src/channels/mount_channel.h"
#include "src/channels/preload.h"

/* 0 = nil, 1 = r/o, 2 = w/o, 3 = r/w */
#define RW_TYPE(channel) \
  (((channel)->limits[GetsLimit] && (channel)->limits[GetSizeLimit]) \
  | ((channel)->limits[PutsLimit] && (channel)->limits[PutSizeLimit]) << 1)

static int disable_preallocation = 0;

void PreloadAllocationDisable()
{
  disable_preallocation = 1;
}

enum ChannelSourceType GetChannelSource(const char *name)
{
  struct stat fs;

  assert(name != NULL);

  /* get the file statistics */
  if(stat(name, &fs) < 0) return ChannelRegular;

  /* calculate the file source type */
  if(S_ISREG(fs.st_mode)) return ChannelRegular;
  if(S_ISDIR(fs.st_mode)) return ChannelDirectory;
  if(S_ISCHR(fs.st_mode)) return ChannelCharacter;
  if(S_ISBLK(fs.st_mode)) return ChannelBlock;
  if(S_ISFIFO(fs.st_mode)) return ChannelFIFO;
  if(S_ISLNK(fs.st_mode)) return ChannelLink;
  if(S_ISSOCK(fs.st_mode)) return ChannelSocket;

  ZLOGFAIL(1, EFAULT, "cannot detect source type of %s", name);
  return ChannelRegular; /* not reachable */
}

int PreloadChannelDtor(struct ChannelDesc* channel)
{
  int i = 0;

  assert(channel != NULL);

  /* adjust the size of writable channels */
  if(channel->limits[PutSizeLimit] && channel->limits[PutsLimit]
     && channel->source == ChannelRegular)
    i = ftruncate(channel->handle, channel->size);

  /* calculate digest and free the tag */
  if(CHANNELS_ETAG_ENABLED)
  {
    TagDigest(channel->tag, channel->digest);
    TagDtor(channel->tag);
  }

  ZLOGS(LOG_DEBUG, "%s closed with tag = %s, getsize = %ld, "
      "putsize = %ld", channel->alias, channel->digest,
      channel->counters[GetSizeLimit], channel->counters[PutSizeLimit]);

  close(channel->handle);
  return i == 0 ? 0 : -1;
}

/* preallocate channel space if not disabled with "-P" */
static void PreallocateChannel(struct ChannelDesc *channel)
{
  int i;

  if(disable_preallocation) return;
  i = ftruncate(channel->handle, channel->limits[PutSizeLimit]);
  ZLOGFAIL(i == -1, errno, "cannot preallocate %s", channel->alias);
}

/* test the channel for validity */
static void FailOnInvalidFileChannel(const struct ChannelDesc *channel)
{
  /* check for supported source types */
  switch(channel->source)
  {
    case ChannelRegular:
    case ChannelCharacter:
    case ChannelFIFO:
      break;
    default:
      ZLOGFAIL(1, EFAULT, "%s cannot be mounted to %s", channel->name,
          StringizeChannelSourceType(channel->source));
      break;
  }

  ZLOGFAIL(channel->name[0] != '/', EFAULT, "only absolute path allowed");
}

/* preload given character device to channel */
static void CharacterChannel(struct ChannelDesc* channel)
{
  char *mode = NULL;
  int flags = 0;

  assert(channel != NULL);
  ZLOG(LOG_DEBUG, "preload character %s", channel->alias);

  /* calculate open mode */
  switch(RW_TYPE(channel))
  {
    case 1:
      mode = "rb";
      flags = O_RDONLY;
      break;
    case 2:
      mode = "wb";
      flags = O_RDWR;
      break;
    default:
      ZLOGFAIL(1, EINVAL, "%s has invalid i/o type", channel->alias);
      break;
  }

  /* open file */
  channel->handle = open(channel->name, flags | O_NONBLOCK);
  channel->socket = fdopen(channel->handle, mode);
  ZLOGFAIL(channel->socket == NULL, errno, "cannot open %s", channel->alias);

  /* set channel attributes */
  channel->size = 0;
}

/* preload given regular device to channel */
static void RegularChannel(struct ChannelDesc* channel)
{
  assert(channel != NULL);
  ZLOG(LOG_DEBUG, "preload regular %s", channel->alias);

  switch(RW_TYPE(channel))
  {
    case 1: /* read only */
      channel->handle = open(channel->name, O_RDONLY, CHANNEL_RIGHTS);
      ZLOGFAIL(channel->handle == -1, errno, "%s open error", channel->name);
      channel->size = GetFileSize((char*)channel->name);
      break;

    case 2: /* write only. existing file will be overwritten */
      channel->handle = open(channel->name, O_WRONLY|O_CREAT|O_TRUNC, CHANNEL_RIGHTS);
      ZLOGFAIL(channel->handle == -1, errno, "%s open error", channel->name);
      channel->size = 0;
      if(!STREQ(channel->name, DEV_NULL))
        PreallocateChannel(channel);
      break;

    case 3: /* cdr or full random access */
      ZLOGFAIL(channel->type == SGetSPut, EFAULT,
          "sequential channels cannot have r/w access");
      ZLOGFAIL(channel->type == SGetRPut, EFAULT,
          "sequential read / random write channels not supported");

      /* open the file and ensure that putpos is not greater than the file size */
      channel->handle = open(channel->name, O_RDWR | O_CREAT, CHANNEL_RIGHTS);
      ZLOGFAIL(channel->handle == -1, errno, "%s open error", channel->name);
      channel->size = GetFileSize(channel->name);
      ZLOGFAIL(channel->putpos > channel->size, EFAULT,
          "%s size is less then specified append position", channel->name);

      /* file does not exist */
      if(channel->size == 0 && !STREQ(channel->name, DEV_NULL))
        PreallocateChannel(channel);
      /* existing file */
      else
        channel->putpos = channel->type == RGetSPut ? channel->size : 0;
      break;

    default:
      ZLOGFAIL(1, EINVAL, "%s has invalid i/o type", channel->alias);
      break;
  }

  ZLOGFAIL(channel->handle < 0, EFAULT, "%s preload error", channel->alias);
}

int PreloadChannelCtor(struct ChannelDesc* channel)
{
  assert(channel != NULL);
  assert(channel->name != NULL);

  /* check the given channel */
  ZLOG(LOG_DEBUG, "mounting file %s to alias %s", channel->name, channel->alias);
  FailOnInvalidFileChannel(channel);

  /* set start position */
  channel->getpos = 0;
  channel->putpos = 0;

  switch(channel->source)
  {
    case ChannelRegular:
      RegularChannel(channel);
      break;
    case ChannelCharacter:
    case ChannelFIFO:
      CharacterChannel(channel);
      break;
    default:
      ZLOGFAIL(1, EPROTONOSUPPORT, "invalid %s source type %d",
          channel->alias, channel->type);
      break;
  }
  return 0;
}
