/*
 * premap user file (channel)
 * since this class only contain startup initializations
 * any error automatically implies abort
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "src/service_runtime/include/bits/mman.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/nacl_config.h"
#include "src/service_runtime/sel_mem.h"
#include "src/service_runtime/nacl_memory_object.h"
#include "src/platform/nacl_log.h"
#include "src/manifest/zmq_syscalls.h"
#include "src/manifest/premap.h"
#include "src/manifest/manifest_setup.h"

#include "src/desc/nacl_desc_io.h"
#include "src/service_runtime/nacl_syscall_common.h"
#include "src/manifest/mount_channel.h"

/* ### remove code doubling
 * infere file open flags by channel prefix
 */
static int GetChannelOpenFlags(struct PreOpenedFileDesc* channel)
{
  int flags[] = CHANNEL_OPEN_FLAGS;
  COND_ABORT(channel->type >= sizeof(flags)/sizeof(*flags), "unknown channel type\n");
  return flags[channel->type];
}

/* ### remove code doubling
 * infere file map flags by channel prefix
 */
static int GetChannelMapFlags(struct PreOpenedFileDesc* channel)
{
  int flags[] = CHANNEL_MAP_FLAGS;
  COND_ABORT(channel->type >= sizeof(flags)/sizeof(*flags), "unknown channel type\n");
  return flags[channel->type];
}

/* ### remove code doubling
 * infere file map protection by channel prefix
 */
static int GetChannelMapProt(struct PreOpenedFileDesc* channel)
{
  int flags[] = CHANNEL_MAP_PROT;
  COND_ABORT(channel->type >= sizeof(flags)/sizeof(*flags), "unknown channel type\n");
  return flags[channel->type];
}

/*
 * preallocate channel. for output files only. since we cannot say how much user
 * program will use we only can allocate max size provided for current channel
 * note: must be called from PremapChannel() after file opened and measured
 * note: user_log file will be trimmed to asciiz string size it contain
 */
static void PreallocateChannel(struct PreOpenedFileDesc* channel)
{
  if(channel->fsize != channel->max_size &&
      (channel->type == OutputChannel || channel->type == LogChannel))
  {
    int ret_code = ftruncate(channel->handle, channel->max_size);
    COND_ABORT(ret_code < 0, "cannot set the channel size\n");
    channel->fsize = channel->max_size;
  }
}

/*
 * premap given file (channel). return 0 if success, otherwise negative errcode
 * note: malloc()
 */
int PremapChannel(struct NaClApp *nap, struct PreOpenedFileDesc* channel)
{
  int desc;
  struct NaClHostDesc *hd = malloc(sizeof(*hd));

  /* debug checks */
  COND_ABORT(!hd, "cannot allocate memory to hold channel descriptor\n");
  COND_ABORT(!channel, "channel is not constructed\n");
  COND_ABORT(channel->mounted != MAPPED, "channel is not supposed to be mapped\n");
  COND_ABORT(!channel->name, "cannot resolve channel name\n");

  /* open file */
  channel->handle = open((char*)channel->name, GetChannelOpenFlags(channel), S_IRWXU);
  COND_ABORT(channel->handle < 0, "channel open error\n");

  /* check if given file in bounds of manifest limits */
  channel->fsize = GetFileSize((char*)channel->name);
  PreallocateChannel(channel);
  COND_ABORT(channel->max_size < channel->fsize, "channel legnth exceeded policy limit\n");

  /* construct nacl descriptor */
  hd->d = channel->handle;
  desc = NaClSetAvail(nap, ((struct NaClDesc *) NaClDescIoDescMake(hd)));

  /* map whole file into the memory. address cannot be higher than stack */
  channel->buffer = NaClCommonSysMmapIntern(nap, NULL, channel->fsize,
      GetChannelMapProt(channel), GetChannelMapFlags(channel), desc, 0);
  COND_ABORT((uint32_t)channel->buffer > 0xFF000000, "channel map error\n");


  /* mounting finalization */
  close(channel->handle);
  channel->bsize = channel->fsize; /* yes. mapped file always put to memory whole */
  channel->handle = -1; /* there is no opened file for mapped channel */

  return 0;
}
