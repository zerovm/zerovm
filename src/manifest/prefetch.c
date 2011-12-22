/*
 * preallocate network channel
 * note: reserved for a future zmq integration
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */

#include "src/service_runtime/sel_ldr.h"
#include "src/manifest/manifest_parser.h"
#include "src/service_runtime/nacl_config.h"
#include "src/service_runtime/sel_mem.h"
#include "src/service_runtime/nacl_memory_object.h"
#include "src/platform/nacl_log.h"

#include <src/manifest/manifest_parser.h>
#include <src/manifest/manifest_setup.h>

/* ###
 * preallocate given network channel. return 0 if success, otherwise negative errcode
 */
int PrefetchChannel(struct NaClApp *nap, struct PreOpenedFileDesc* channel)
{
  // place to put zeromq calls
  return -1;
}
