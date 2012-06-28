/*
 * premap user file to channel
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "src/utils/tools.h"
#include "src/desc/nacl_desc_io.h"
#include "src/service_runtime/include/bits/mman.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/nacl_syscall_handlers.h"
#include "src/manifest/premap.h"

static int open_flags[] = CHANNEL_OPEN_FLAGS;
static int map_flags[] = CHANNEL_MAP_FLAGS;
static int map_prot[] = CHANNEL_MAP_PROT;

#define OPEN_FLAGS(ch) open_flags[ch] /* get open flags by channel type */
#define MAP_FLAGS(ch) map_flags[ch] /* get mapping flags by channel type */
#define OPEN_PROT(ch) map_prot[ch] /* get protection mode by channel type */

/*
 * preallocate channel. for output files only. since we cannot say how much user
 * program will use we only can allocate max size provided for current channel
 * note: must be called from PremapChannel() after file opened and measured
 */
static void PreallocateChannel(struct ChannelDesc* channel)
{
  assert(channel != NULL);

  if(channel->fsize != channel->limits[PutSizeLimit] &&
      (channel->type == OutputChannel || channel->type == LogChannel))
  {
    int ret_code = ftruncate(channel->handle, channel->limits[PutSizeLimit]);
    COND_ABORT(ret_code < 0, "cannot set the file size for mapped channel");
    channel->fsize = channel->limits[PutSizeLimit];
  }
}

/*
 * premap given file (channel). return 0 if success, otherwise negative errcode
 * note: malloc()
 */
int PremapChannel(struct NaClApp *nap, struct ChannelDesc* channel)
{
  int desc;
  struct NaClHostDesc *hd = malloc(sizeof(*hd));

  assert(nap != NULL);
  assert(channel != NULL);
  assert(nap != NULL);
  assert(channel->mounted == MAPPED);
  assert((void*)channel->name != NULL);

  /* debug checks */
  COND_ABORT(hd == NULL, "cannot allocate memory to hold premapped channel decs");

  /* open file */
  channel->handle = open((char*)channel->name, OPEN_FLAGS(channel->type), S_IRWXU);
  COND_ABORT(channel->handle < 0, "premapped channel open error");

  /* check if given file in bounds of manifest limits */
  channel->fsize = GetFileSize((char*)channel->name);
  PreallocateChannel(channel);

  /* construct nacl descriptor */
  hd->d = channel->handle;
  desc = NaClSetAvail(nap, ((struct NaClDesc *) NaClDescIoDescMake(hd)));

  /* map whole file into the memory. address cannot be higher than stack */
  /* todo(d'b): replace magic number */
  channel->buffer = NaClCommonSysMmapIntern(nap, NULL, channel->fsize,
      OPEN_PROT(channel->type), MAP_FLAGS(channel->type), desc, 0);
  COND_ABORT((uint32_t)channel->buffer > 0xFF000000, "channel map error");

  /* mounting finalization */
  close(channel->handle);
  channel->bsize = channel->fsize; /* yes. mapped file always put to memory whole */
  channel->handle = -1; /* there is no opened file for mapped channel */

  return OK_CODE;
}
