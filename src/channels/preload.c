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

#include <assert.h>
#include "src/channels/mount_channel.h"
#include "src/channels/preload.h"

/* return the file source type or ChannelSourceTypeNumber */
enum ChannelSourceType GetChannelSource(const char *name)
{
  struct stat fs;
  int desc;
  int code;

  assert(name != NULL);

  /* get the file statistics (try both open modes r/o and w/o) */
  /*
   * todo(d'b): it is assumed not existing files are regular
   * how to support other types like pipes and others?
   */
  desc = open(name, O_RDONLY);
  if(desc < 0) return ChannelRegular;

  code = fstat(desc, &fs);
  close(desc);
  if(code < 0) return ChannelSourceTypeNumber;

  /* calculate the file source type */
  if(S_ISREG(fs.st_mode)) return ChannelRegular;
  if(S_ISDIR(fs.st_mode)) return ChannelDirectory;
  if(S_ISCHR(fs.st_mode)) return ChannelCharacter;
  if(S_ISBLK(fs.st_mode)) return ChannelBlock;
  if(S_ISFIFO(fs.st_mode)) return ChannelFIFO;
  if(S_ISLNK(fs.st_mode)) return ChannelLink;
  if(S_ISSOCK(fs.st_mode)) return ChannelSocket;

  return ChannelSourceTypeNumber;
}

/* (adjust and) close file associated with the channel */
int PreloadChannelDtor(struct ChannelDesc* channel)
{
  int i = OK_CODE;

  assert(channel != NULL);

  /* adjust the size of writable channels */
  if(channel->limits[PutSizeLimit] && channel->limits[PutsLimit])
    i = ftruncate(channel->handle, channel->putpos);

  close(channel->handle);
  return i == 0 ? OK_CODE : ERR_CODE;
}

/*
 * test the channel for validity
 * todo(d'b): add more checks
 */
static void FailOnInvalidFileChannel(const struct ChannelDesc *channel)
{
  ZLOGFAIL(channel->source < ChannelRegular || channel->source > ChannelSocket,
      EFAULT, "'%s' isn't local file", channel->alias);
  ZLOGFAIL(channel->name[0] != '/', EFAULT, "only absolute path allowed");
  ZLOGFAIL(channel->source == ChannelCharacter
      && (channel->limits[PutsLimit] && channel->limits[GetsLimit]),
      EFAULT, "invalid channel limits");
}

/* preload given character device to channel */
static void CharacterChannel(struct ChannelDesc* channel)
{
  char *mode = NULL;

  assert(channel != NULL);

  /* calculate open mode */
  mode = channel->limits[PutsLimit] == 0 ? "rb" : "wb";

  /* open file */
  channel->socket = fopen(channel->name, mode);
  ZLOGFAIL(channel->socket == NULL, errno, "cannot open channel %s", channel->name);

  /* set channel attributes */
  channel->size = 0;
}

/* preload given regular device to channel */
static void RegularChannel(struct ChannelDesc* channel)
{
  uint32_t rw = 0;
  int i;

  assert(channel != NULL);
  assert(channel->name != NULL);

  /* calculate the read/write type */
  rw |= channel->limits[GetsLimit] && channel->limits[GetSizeLimit];
  rw |= (channel->limits[PutsLimit] && channel->limits[PutSizeLimit]) << 1;
  switch(rw)
  {
    case 1: /* read only */
      channel->handle = open(channel->name, O_RDONLY, S_IRWXU);
      ZLOGFAIL(channel->handle == -1, errno, "'%s' open error", channel->name);
      channel->size = GetFileSize((char*)channel->name);
      break;

    case 2: /* write only. existing file will be overwritten */
      channel->handle = open(channel->name, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
      ZLOGFAIL(channel->handle == -1, errno, "'%s' open error", channel->name);
      channel->size = 0;
      if(STREQ(channel->name, DEV_NULL)) break;
      i = ftruncate(channel->handle, channel->limits[PutSizeLimit]);
      ZLOGFAIL(i == -1, errno, "cannot preallocate '%s' channel", channel->alias);
      break;

    case 3: /* cdr or full random access */
      ZLOGFAIL(channel->type == SGetSPut, EFAULT,
          "sequential channels cannot have r/w access");
      ZLOGFAIL(channel->type == SGetRPut, EFAULT,
          "sequential read / random write channels not supported");

      /* open the file and ensure that putpos is not greater than the file size */
      channel->handle = open(channel->name, O_RDWR | O_CREAT, S_IRWXU);
      ZLOGFAIL(channel->handle == -1, errno, "'%s' open error", channel->name);
      channel->size = GetFileSize(channel->name);
      ZLOGFAIL(channel->putpos > channel->size, EFAULT,
          "'%s' size is less then specified append position", channel->alias);

      /* file does not exist */
      if(channel->size == 0 && STRNEQ(channel->name, DEV_NULL))
      {
        i = ftruncate(channel->handle, channel->limits[PutSizeLimit]);
        ZLOGFAIL(i == -1, errno, "cannot preallocate cdr channel");
        break;
      }

      /* update the start of writing position */
      channel->putpos = channel->type == RGetSPut ? channel->size : 0;
      break;

    case 0: /* inaccessible */
    default: /* unreachable */
      ZLOGFAIL(1, EPROTONOSUPPORT, "the channel '%s' not supported", channel->alias);
      break;
  }

  ZLOGFAIL(channel->handle < 0, EFAULT, "preloaded file open error");
}

/*
 * preload given file to channel.
 * return 0 if success, otherwise negative errcode
 */
int PreloadChannelCtor(struct ChannelDesc* channel)
{
  assert(channel != NULL);
  assert(channel->name != NULL);

  /* check the given channel */
  ZLOGS(LOG_DEBUG, "mounting channel '%s' to '%s'", channel->name, channel->alias);
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
    case ChannelSocket:
      CharacterChannel(channel);
      break;
    default:
      ZLOGFAIL(1, EPROTONOSUPPORT, "the channel not supported");
      break;
  }
  return OK_CODE;
}
