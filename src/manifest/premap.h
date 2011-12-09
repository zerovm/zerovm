/*
 * premap user file (channel) header
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */

#ifndef PREMAP_H_
#define PREMAP_H_
#include "src/service_runtime/sel_ldr.h"
#include "api/zvm_manifest.h"

/* open/map flags/modes. must contain "channel types" number of elements -1 means absense of value */
#define CHANNEL_OPEN_FLAGS {O_RDONLY, O_RDWR | O_CREAT , O_RDWR | O_CREAT , -1, -1}
#define CHANNEL_MAP_FLAGS {NACL_ABI_MAP_PRIVATE, NACL_ABI_MAP_SHARED, NACL_ABI_MAP_SHARED, -1, -1}
#define CHANNEL_MAP_PROT {NACL_ABI_PROT_READ, NACL_ABI_PROT_WRITE, NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE, -1, -1}

/*
 * premap given file (channel). return 0 if success, otherwise negative errcode
 */
int PremapChannel(struct NaClApp *nap, struct PreOpenedFileDesc* channel);

#endif /* PREMAP_H_ */
