/*
 * preload given file to channel.
 * note: with this class we make paging engine
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */
#include <assert.h>
#include "src/manifest/mount_channel.h"
#include "src/manifest/preload.h"

/* return the file source type or ChannelSourceTypeNumber */
enum ChannelSourceType GetChannelSource(const char *name)
{
  struct stat fs;
  int desc;
  int code;

  assert(name != NULL);

  /* get the file statistics (try both open modes r/o and w/o) */
  desc = open(name, O_RDONLY);
  if(desc < 0) desc = open(name, O_WRONLY);
  if(desc < 0) desc = open(name, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
  if(desc < 0) return ChannelSourceTypeNumber;
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
  FailIf(channel->source < ChannelRegular || channel->source > ChannelSocket,
      "'%s' isn't local file", channel->alias);
  FailIf(channel->name[0] != '/', "only absolute path allowed");
  FailIf(channel->source == ChannelCharacter
      && (channel->limits[PutsLimit] && channel->limits[GetsLimit]),
      "invalid channel limits");
}

/* preload given character device to channel */
static void CharacterChannel(struct ChannelDesc* channel)
{
  char *mode = NULL;

  assert(channel != NULL);
  assert(channel->source == ChannelCharacter);

  /* calculate open mode */
  mode = channel->limits[PutsLimit] == 0 ? "rb" : "wb";

  /* open file */
  channel->socket = fopen(channel->name, mode);
  FailIf(channel->socket == NULL, "cannot open channel %s", channel->name);

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
      channel->size = GetFileSize((char*)channel->name);
      break;
    case 2: /* write only. existing file will be overwritten */
      channel->handle = open(channel->name, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
      channel->size = 0;
      if(STREQ(channel->name, DEV_NULL)) break;
      i = ftruncate(channel->handle, channel->limits[PutSizeLimit]);
      FailIf(i != 0, "cannot preallocate '%s' channel", channel->alias);
      break;
    case 3: /* cdr */
      FailIf(channel->type != 1, "only cdr channels can have r/w access");

      /* open the file and ensure that putpos is not greater than the file size */
      channel->handle = open(channel->name, O_RDWR|O_CREAT, S_IRWXU);
      channel->size = GetFileSize(channel->name);
      FailIf(channel->putpos > channel->size,
          "'%s' size is less then specified append position", channel->alias);

      /* file does not exist */
      if(channel->size == 0 && STRNEQ(channel->name, DEV_NULL))
      {
        i = ftruncate(channel->handle, channel->limits[PutSizeLimit]);
        FailIf(i != 0, "cannot preallocate cdr channel");
        break;
      }

      /* update putpos preferring to use manifest value */
      if(channel->putpos == 0) channel->putpos = channel->size;
      break;

    case 0: /* inaccessible */
    default: /* unreachable */
      ZLOG(LOG_FATAL, "the channel '%s' not supported", channel->alias);
      break;
  }

  FailIf(channel->handle < 0, "preloaded file open error");
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
  ZLOG(LOG_DEBUG, "mounting channel '%s' to '%s'", channel->name, channel->alias);
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
      CharacterChannel(channel);
      break;
    default:
      ZLOG(LOG_FATAL, "the channel not supported");
      break;
  }
  return OK_CODE;
}
