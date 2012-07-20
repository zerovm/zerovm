/*
 * preallocate network channel
 * todo(NETWORKING): under construction
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */
#include <assert.h>
#include "api/zvm.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/manifest/manifest_parser.h"
#include "src/manifest/mount_channel.h"
#include "src/service_runtime/nacl_config.h"
#include "src/service_runtime/sel_mem.h"
#include "src/service_runtime/nacl_memory_object.h"
#include "src/platform/nacl_log.h"

/* from sel_main.c */
#include "src/networking/zvm_netw.h"
#include "src/networking/zmq_netw.h" /* SockCapability */
#define ZVM_DB_NAME "zvm_netw.db"


/* todo(d'b): under construction */
int PrefetchChannelCtor(struct ChannelDesc* channel)
{
  assert(channel != NULL);

  return ERR_CODE;
}

/* todo(d'b): under construction */
int PrefetchChannelDtor(struct ChannelDesc* channel)
{
  assert(channel != NULL);

  return ERR_CODE;
}

