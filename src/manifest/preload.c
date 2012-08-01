/*
 * preload given file to channel.
 * note: with this class we make paging engine
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "api/zvm.h"
#include "src/utils/tools.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/manifest/mount_channel.h"
#include "src/manifest/preload.h"

/* todo(d'b): under construction */
int PreloadChannelDtor(struct ChannelDesc* channel)
{
  assert(channel != NULL);

  ftruncate(channel->handle, channel->putpos);
  close(channel->handle);
  return OK_CODE;
}

/*
 * preload given file to channel.
 * return 0 if success, otherwise negative errcode
 */
int PreloadChannelCtor(struct ChannelDesc* channel)
{
  uint32_t rw = 0;

  assert(channel != NULL);
  assert(channel->name != NULL);

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
    case 2: /* write only */
      channel->handle = open(channel->name, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
      posix_fallocate(channel->handle, channel->putpos, channel->limits[PutSizeLimit]);
      channel->size = 0;
      break;
    case 3: /* read/write and cdr (append) */
      channel->handle = open(channel->name, O_RDWR|O_CREAT, S_IRWXU);
      channel->size = GetFileSize(channel->name);
      COND_ABORT(channel->putpos > channel->size,
          "cdr channel size is less then specified append position");

      /* file does not exist */
      if(channel->size == 0)
      {
        posix_fallocate(channel->handle, channel->putpos, channel->limits[PutSizeLimit]);
        channel->size = 0;
      }
      /* file exists */
      else
      {
        /* set append position for cdr */
        if(channel->putpos > 0) break; /* prefer to use manifest value */
        channel->putpos = channel->type == RGetSPut && channel->size > 0 ?
            channel->size : 0;
      }
      break;

    default: /* unreachable */
      return ERR_CODE;
  }

  COND_ABORT(channel->handle < 0, "preloaded file open error");

  return OK_CODE;
}
