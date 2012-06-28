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
#include "src/manifest/preload.h"

static int flags[] = CHANNEL_OPEN_FLAGS;
#define FLAGS(ch) flags[ch] /* get open flags by channel type */

/*
 * preallocate channel. if size of the file is not 0 - skip. for output files only.
 * note: must be called from PreloadChannel() after file opened and measured
 */
static void PreallocateChannel(struct ChannelDesc* channel)
{
  assert(channel != NULL);

  if(channel->fsize < 1 &&
      (channel->type == OutputChannel || channel->type == LogChannel))
  {
    int ret_code = ftruncate(channel->handle, 0);
    COND_ABORT(ret_code < 0, "cannot extend file for the preloaded channel");
    channel->fsize = channel->limits[PutSizeLimit];
  }
}

/*
 * preload given file to channel.
 * return 0 if success, otherwise negative errcode
 */
int PreloadChannel(struct NaClApp *nap, struct ChannelDesc* channel)
{
  assert(nap != NULL);
  assert(channel != NULL);
  assert(nap != NULL);
  assert(channel->mounted == LOADED);

  /* if channel is not constructed. skip it */
  if((void*)channel->name == NULL) return ERR_CODE;

  /* open file and allocate channel */
  channel->handle = open((char*)channel->name, FLAGS(channel->type), S_IRWXU);
  COND_ABORT(channel->handle < 0, "preloaded file open error");
  channel->fsize = GetFileSize((char*)channel->name);
  PreallocateChannel(channel);

  /* mounting finalization */
  channel->bsize = -1; /* will be provided by user */
  return OK_CODE;
}
