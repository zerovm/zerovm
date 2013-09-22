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
#include "src/channels/preload.h"

#define CHANNEL_RIGHTS S_IRUSR | S_IWUSR
#define DEV_NULL "/dev/null"

static int disable_preallocation = 0;

void PreloadAllocationDisable()
{
  disable_preallocation = 1;
}

/* detect and set source type */
#define SET(f, p) if(f(fs.st_mode)) CH_PROTO(channel, n) = Proto##p; else
static void SetChannelSource(struct ChannelDesc *channel, int n)
{
  struct stat fs;

  assert(channel != NULL);
  assert(CH_NAME(channel, n) != NULL);

  /*
   * get the file statistics and calculate the source type (if file
   * is not exist it will be regular)
   */
  if(stat(CH_NAME(channel, n), &fs) < 0) CH_PROTO(channel, n) = ProtoRegular; else
  SET(S_ISREG, Regular)
  SET(S_ISDIR, Directory)
  SET(S_ISCHR, Character)
  SET(S_ISBLK, Block)
  SET(S_ISFIFO, FIFO)
  SET(S_ISLNK, Link)
  SET(S_ISSOCK, Socket)
  ZLOGFAIL(1, EFAULT, "cannot detect source type of %s", CH_NAME(channel, n));
}

int PreloadChannelDtor(struct ChannelDesc *channel, int n)
{
  int code = 0;
  int handle;

  assert(channel != NULL);
  assert(n < channel->source->len);

  /* adjust the size of writable channels */
  handle = GPOINTER_TO_INT(CH_HANDLE(channel, n));
  if(channel->limits[PutSizeLimit] && channel->limits[PutsLimit]
     && CH_PROTO(channel, n) == ProtoRegular)
    code = ftruncate(handle, channel->size);

  ZLOGS(LOG_DEBUG,
      "%s closed with getsize = %ld, putsize = %ld", channel->alias,
      channel->counters[GetSizeLimit], channel->counters[PutSizeLimit]);

  if(handle != 0)
  {
    if(CH_PROTO(channel, n) == ProtoRegular)
      close(handle);
    else
      fclose(CH_HANDLE(channel, n));
  }

  g_ptr_array_remove(channel->source, CH_FILE(channel, n));
  return -(code != 0);
}

/* preallocate channel space if not disabled with "-P" */
static void PreallocateChannel(const struct ChannelDesc *channel, int n)
{
  int code;
  int handle = GPOINTER_TO_INT(CH_HANDLE(channel, n));

  if(disable_preallocation) return;
  code = ftruncate(handle, channel->limits[PutSizeLimit]);
  ZLOGFAIL(code == -1, errno, "cannot preallocate %s", channel->alias);
}

/* preload given character/FIFO device to channel */
static void CharacterChannel(struct ChannelDesc* channel, int n)
{
  char *mode[] = {NULL, "rb", "wb"};
  int flags[] = {0, O_RDONLY, O_RDWR};
  int h;

  ZLOG(LOG_DEBUG, "preload character %s", channel->alias);
  ZLOGFAIL(!IS_RO(channel) && !IS_WO(channel), EINVAL,
      "%s has invalid i/o type", channel->alias);

  /* open file */
  h = open(CH_NAME(channel, n), flags[CH_RW_TYPE(channel)]);
  CH_HANDLE(channel, n) = fdopen(h, mode[CH_RW_TYPE(channel)]);
  ZLOGFAIL(CH_HANDLE(channel, n) == NULL, errno,
      "cannot open %s", CH_NAME(channel, n));

  /* set channel attributes */
  channel->size = 0;
}

/* preload given regular device to channel */
static void RegularChannel(struct ChannelDesc* channel, int n)
{
  int h;
  ZLOG(LOG_DEBUG, "preload regular %s", channel->alias);

  switch(CH_RW_TYPE(channel))
  {
    case 1: /* read only */
      h = open(CH_NAME(channel, n), O_RDONLY, CHANNEL_RIGHTS);
      CH_HANDLE(channel, n) = GINT_TO_POINTER(h);
      channel->size = GetFileSize(CH_NAME(channel, n));
      break;

    case 2: /* write only. existing file will be overwritten */
      h = open(CH_NAME(channel, n), O_WRONLY|O_CREAT|O_TRUNC, CHANNEL_RIGHTS);
      CH_HANDLE(channel, n) = GINT_TO_POINTER(h);
      channel->size = 0;
      if(g_strcmp0(CH_NAME(channel, n), DEV_NULL) != 0)
        PreallocateChannel(channel, n);
      break;

    case 3: /* cdr or full random access */
      ZLOGFAIL(channel->type == SGetSPut, EFAULT,
          "sequential channels cannot have r/w access");
      ZLOGFAIL(channel->type == SGetRPut, EFAULT,
          "sequential read / random write channels not supported");

      /* open the file and ensure that putpos is not greater than the file size */
      h = open(CH_NAME(channel, n), O_RDWR | O_CREAT, CHANNEL_RIGHTS);
      CH_HANDLE(channel, n) = GINT_TO_POINTER(h);
      channel->size = GetFileSize(CH_NAME(channel, n));
      ZLOGFAIL(channel->putpos > channel->size, EFAULT,
          "%s size is less then specified append position", CH_NAME(channel, n));

      /* file does not exist */
      if(channel->size == 0 && g_strcmp0(CH_NAME(channel, n), DEV_NULL) != 0)
        PreallocateChannel(channel, n);
      /* existing file */
      else
        channel->putpos = channel->type == RGetSPut ? channel->size : 0;
      break;

    default:
      ZLOGFAIL(1, EINVAL, "%s has invalid i/o type", channel->alias);
      break;
  }

  ZLOGFAIL(GPOINTER_TO_INT(CH_HANDLE(channel, n)) < 0,
      errno, "%s open error", CH_NAME(channel, n));
}

void PreloadChannelCtor(struct ChannelDesc *channel, int n)
{
  assert(channel != NULL);
  assert(n < channel->source->len);

  /* check the given channel */
  ZLOG(LOG_DEBUG, "mounting file %s to alias %s",
      CH_NAME(channel, n), channel->alias);

  SetChannelSource(channel, n);

  switch(CH_PROTO(channel, n))
  {
    case ProtoRegular:
      RegularChannel(channel, n);
      break;
    case ProtoCharacter:
    case ProtoFIFO:
      CharacterChannel(channel, n);
      break;
    default:
      ZLOGFAIL(1, EPROTONOSUPPORT, "invalid %s source type %d",
          channel->alias, channel->type);
      break;
  }
}
