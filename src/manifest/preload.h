/*
 * preload user file (channel) header
 * note: with this class we make paging engine
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */

#ifndef PRELOAD_H_
#define PRELOAD_H_

/* remove junk */
#include "src/service_runtime/include/bits/mman.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/manifest/manifest_parser.h"
#include "src/service_runtime/nacl_config.h"
#include "src/service_runtime/sel_mem.h"
#include "src/service_runtime/nacl_memory_object.h"
#include "src/platform/nacl_log.h"
#include <src/manifest/manifest_parser.h>
#include <src/manifest/manifest_setup.h>
/**/

#define CHANNEL_OPEN_FLAGS {O_RDONLY, O_WRONLY | O_CREAT, O_WRONLY | O_CREAT, -1, -1}
/*
 * preload given file (channel). return 0 if success, otherwise negative errcode
 */
int PreloadChannel(struct NaClApp *nap, struct PreOpenedFileDesc* channel);

#endif /* PRELOAD_H_ */
