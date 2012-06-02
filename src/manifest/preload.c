/*
 * preload user file (channel)
 * since this class only contain startup initializations
 * any error automatically implies abort
 * note: with this class we make paging engine
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */
#include <stdio.h>
#include <fcntl.h>

#include <src/manifest/preload.h>
#include "src/manifest/mount_channel.h"

/*
 * infere file open flags by channel prefix
 */
static int GetChannelOpenFlags(struct PreOpenedFileDesc* channel)
{
  int flags[] = CHANNEL_OPEN_FLAGS;
  COND_ABORT(channel->type >= sizeof(flags)/sizeof(*flags), "unknown channel type\n");
  return flags[channel->type];
}

/*
 * preallocate channel. if size of the file is not 0 - skip. for output files only.
 * note: must be called from PreloadChannel() after file opened and measured
 */
static void PreallocateChannel(struct PreOpenedFileDesc* channel)
{
  if(channel->fsize < 1 &&
      (channel->type == OutputChannel || channel->type == LogChannel))
  {
    int ret_code = ftruncate(channel->handle, 0);
    COND_ABORT(ret_code < 0, "cannot extend the channel\n");
    channel->fsize = channel->max_size;
  }
}

/*
 * preload given file (channel). return 0 if success, otherwise negative errcode
 */
int PreloadChannel(struct NaClApp *nap, struct PreOpenedFileDesc* channel)
{
  /* debug checks */
  if(!channel->name) return -1; /* channel is not constructed. skip it */
  COND_ABORT(channel->mounted != LOADED, "channel is not supposed to be loaded\n");

  /* open file */
  channel->handle = open((char*)channel->name, GetChannelOpenFlags(channel), S_IRWXU);
  COND_ABORT(channel->handle < 0, "channel open error\n");

  /* check if given file in bounds of manifest limits */
  channel->fsize = GetFileSize((char*)channel->name);
  PreallocateChannel(channel);
  COND_ABORT(channel->max_size < channel->fsize,
             "channel legnth exceeded policy limit\n");

  /* mounting finalization */
  channel->bsize = -1; /* will be provided by user */
  return 0;
}
