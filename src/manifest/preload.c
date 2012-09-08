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
  COND_ABORT(channel->source != LocalFile, "channel isn't LocalFile");
  COND_ABORT(channel->name[0] != '/', "only absolute path allowed");
}

/*
 * preload given file to channel.
 * return 0 if success, otherwise negative errcode
 */
int PreloadChannelCtor(struct ChannelDesc* channel)
{
  uint32_t rw = 0;
  int i;

  assert(channel != NULL);
  assert(channel->name != NULL);

  /* check the given channel */
  FailOnInvalidFileChannel(channel);

  /* set start position */
  channel->getpos = 0; /* todo(d'b): add attribute to manifest? */
  channel->putpos = 0; /* todo(d'b): add attribute to manifest? */

  /* calculate the read/write type */
  rw |= channel->limits[GetsLimit] && channel->limits[GetSizeLimit];
  rw |= (channel->limits[PutsLimit] && channel->limits[PutSizeLimit]) << 1;
  switch(rw)
  {
    case 0: /* inaccessible */
      return ERR_CODE;
    case 1: /* read only */
      channel->handle = open(channel->name, O_RDONLY, S_IRWXU);
      channel->size = GetFileSize((char*)channel->name);
      break;
    case 2: /* write only. existing file will be overwritten */
      channel->handle = open(channel->name, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
      channel->size = 0;
      if(STREQ(channel->name, DEV_NULL)) break;
      i = ftruncate(channel->handle, channel->limits[PutSizeLimit]);
      COND_ABORT(i != 0, "cannot preallocate w/o channel");
      break;
    case 3: /* cdr */
      COND_ABORT(channel->type != 1, "only cdr channels can have r/w access");

      /* open the file and ensure that putpos is not greater than the file size */
      channel->handle = open(channel->name, O_RDWR|O_CREAT, S_IRWXU);
      channel->size = GetFileSize(channel->name);
      COND_ABORT(channel->putpos > channel->size,
          "cdr channel size is less then specified append position");

      /* file does not exist */
      if(channel->size == 0 && STRNEQ(channel->name, DEV_NULL))
      {
        i = ftruncate(channel->handle, channel->limits[PutSizeLimit]);
        COND_ABORT(i != 0, "cannot preallocate cdr channel");
        break;
      }

      /* update putpos preferring to use manifest value */
      if(channel->putpos == 0) channel->putpos = channel->size;
      break;

    default: /* unreachable */
      return ERR_CODE;
  }

  COND_ABORT(channel->handle < 0, "preloaded file open error");

  return OK_CODE;
}
