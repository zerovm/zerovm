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

/* remove junk */
#include "src/service_runtime/include/bits/mman.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/manifest/manifest_parser.h"
#include "src/service_runtime/nacl_config.h"
#include "src/service_runtime/sel_mem.h"
#include "src/service_runtime/nacl_memory_object.h"
#include "src/platform/nacl_log.h"
#include "src/manifest/zmq_syscalls.h"
#include <src/manifest/manifest_parser.h>
#include <src/manifest/manifest_setup.h>
/**/

#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/nacl_config.h"
#include "src/platform/nacl_log.h"
#include "src/manifest/zmq_syscalls.h" /* only GetFileSize() used ### move it utlis? */
#include <src/manifest/preload.h>
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

/*
 * preload given file (channel). return 0 if success, otherwise negative errcode
 */
int PreloadChannel(struct NaClApp *nap, struct PreOpenedFileDesc* channel)
{
  /* debug checks */
  if(!channel->name) return -1; /* channel is not constructed. skip it */
  COND_ABORT(channel->mounted != LOADED, "channel is not supposed to be loaded\n");

  /* open file */
  // ### add O_DIRECT needs #define _GNU_SOURCE
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
