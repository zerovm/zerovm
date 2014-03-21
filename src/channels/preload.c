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
#include <stdio.h>
#include "src/main/zlog.h"
#include "src/channels/preload.h"

#define CHANNEL_RIGHTS S_IRUSR | S_IWUSR
#define DEV_NULL "/dev/null"

static int disable_preallocation = 0;

void PreloadAllocationDisable()
{
  disable_preallocation = 1;
}

/* detect and set source type */
#define SET(f, p) if(f(fs.st_mode)) channel->protocol = Proto##p; else
static void SetChannelSource(struct ChannelDesc *channel)
{
  struct stat fs;

  assert(channel != NULL);

  /*
   * get the file statistics and calculate the source type (if file
   * is not exist it will be regular)
   */
  if(stat(channel->name, &fs) < 0) channel->protocol = ProtoRegular; else
  SET(S_ISREG, Regular)
  SET(S_ISDIR, Directory)
  SET(S_ISCHR, Character)
  SET(S_ISBLK, Block)
  SET(S_ISFIFO, FIFO)
  SET(S_ISLNK, Link)
  SET(S_ISSOCK, Socket)
  ZLOGFAIL(1, EFAULT, "cannot detect source type of %s", channel->name);
}

int PreloadChannelDtor(struct ChannelDesc *channel)
{
  int code = 0;
  int handle;

  assert(channel != NULL);

  /* adjust the size of writable channels */
  handle = GPOINTER_TO_INT(channel->handle);
  if(channel->limits[PutSizeLimit] && channel->limits[PutsLimit]
     && channel->protocol == ProtoRegular)
    code = ftruncate(handle, channel->size);

  ZLOGS(LOG_DEBUG,
      "%s closed with getsize = %ld, putsize = %ld", channel->alias,
      channel->counters[GetSizeLimit], channel->counters[PutSizeLimit]);

  if(handle != 0)
  {
    if(channel->protocol == ProtoRegular)
      close(handle);
    else
      fclose(channel->handle);
  }

  return code;
}

/* preallocate channel space if not disabled with "-P" */
static void PreallocateChannel(const struct ChannelDesc *channel)
{
  int code;
  int handle = GPOINTER_TO_INT(channel->handle);

  if(disable_preallocation) return;
  code = ftruncate(handle, channel->limits[PutSizeLimit]);
  ZLOGFAIL(code == -1, errno, "cannot preallocate %s", channel->alias);
}

/* preload given character/FIFO device to channel */
static void CharacterChannel(struct ChannelDesc* channel)
{
  char *mode[] = {NULL, "rb", "wb"};
  int flags[] = {0, O_RDONLY, O_RDWR};
  int h;

  ZLOGS(LOG_DEBUG, "preload character %s", channel->alias);
  ZLOGFAIL(!IS_RO(channel) && !IS_WO(channel), EINVAL,
      "%s has invalid i/o type", channel->alias);

  /* open file */
  h = open(channel->name, flags[CH_RW_TYPE(channel)]);
  channel->handle = fdopen(h, mode[CH_RW_TYPE(channel)]);
  ZLOGFAIL(channel->handle == NULL, errno, "cannot open %s", channel->name);

  /* set channel attributes */
  channel->size = 0;
}

/* preload given regular device to channel */
static void RegularChannel(struct ChannelDesc* channel)
{
  int h;
  ZLOGS(LOG_DEBUG, "preload regular %s", channel->alias);

  switch(CH_RW_TYPE(channel))
  {
    case 1: /* read only */
      h = open(channel->name, O_RDONLY, CHANNEL_RIGHTS);
      channel->handle = GINT_TO_POINTER(h);
      channel->size = GetFileSize(channel->name);
      ZLOGFAIL(channel->size < 0, EFAULT, "cannot open %s", channel->name);
      break;

    case 2: /* write only. existing file will be overwritten */
      h = open(channel->name, O_WRONLY|O_CREAT|O_TRUNC, CHANNEL_RIGHTS);
      channel->handle = GINT_TO_POINTER(h);
      channel->size = 0;
      if(g_strcmp0(channel->name, DEV_NULL) != 0)
        PreallocateChannel(channel);
      break;

    case 3: /* cdr or full random access */
      ZLOGFAIL(channel->type == SGetSPut, EFAULT,
          "sequential channels cannot have r/w access");
      ZLOGFAIL(channel->type == SGetRPut, EFAULT,
          "sequential read / random write channels not supported");

      /* open file and ensure that putpos is not greater than the file size */
      h = open(channel->name, O_RDWR | O_CREAT, CHANNEL_RIGHTS);
      channel->handle = GINT_TO_POINTER(h);
      channel->size = GetFileSize(channel->name);
      ZLOGFAIL(channel->size < 0, EFAULT, "cannot open %s", channel->name);
      ZLOGFAIL(channel->putpos > channel->size, EFAULT,
          "%s size is less then specified append position", channel->name);

      /* file does not exist */
      if(channel->size == 0 && g_strcmp0(channel->name, DEV_NULL) != 0)
        PreallocateChannel(channel);
      /* existing file */
      else
        channel->putpos = channel->type == RGetSPut ? channel->size : 0;
      break;

    default:
      ZLOGFAIL(1, EINVAL, "%s has invalid i/o type", channel->alias);
      break;
  }

  ZLOGFAIL(GPOINTER_TO_INT(channel->handle) < 0,
      errno, "%s open error", channel->name);
}

void PreloadChannelCtor(struct ChannelDesc *channel)
{
  assert(channel != NULL);

  /* check the given channel */
  ZLOGS(LOG_DEBUG, "mounting file %s to alias %s",
      channel->name, channel->alias);

  SetChannelSource(channel);

  switch(channel->protocol)
  {
    case ProtoRegular:
      RegularChannel(channel);
      break;
    case ProtoCharacter:
    case ProtoFIFO:
      CharacterChannel(channel);
      break;
    default:
      ZLOGFAIL(1, EPROTONOSUPPORT, "invalid %s source type %d",
          channel->alias, channel->type);
      break;
  }
}
