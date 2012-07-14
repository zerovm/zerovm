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
  assert(channel->name != (int64_t)NULL);

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
      channel->handle = open((char*)channel->name, O_RDONLY, S_IRWXU);
      break;
    case 2: /* write only */
      channel->handle = open((char*)channel->name, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
      posix_fallocate(channel->handle, channel->putpos, channel->limits[PutSizeLimit]);
      break;
    case 3: /* read/write */
      channel->handle = open((char*)channel->name, O_RDWR|O_CREAT, S_IRWXU);
      posix_fallocate(channel->handle, channel->putpos, channel->limits[PutSizeLimit]);
      break;
    default: /* unreachable */
      return ERR_CODE;
  }

  COND_ABORT(channel->handle < 0, "preloaded file open error");
  channel->size = GetFileSize((char*)channel->name);

  return OK_CODE;
}
