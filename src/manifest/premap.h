/*
 * premap user file to channel
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */

#ifndef PREMAP_H_
#define PREMAP_H_
#include "src/service_runtime/sel_ldr.h"
#include "api/zvm.h"
#include "src/manifest/mount_channel.h"

/* flags and modes for file mapping. -1 means absense of value */
#undef CHANNEL_OPEN_FLAGS /* prevent warning in mount_channel.c */
#define CHANNEL_OPEN_FLAGS \
  {O_RDONLY, O_RDWR | O_CREAT, O_RDWR | O_CREAT, -1, -1}
#define CHANNEL_MAP_FLAGS \
  {NACL_ABI_MAP_PRIVATE, NACL_ABI_MAP_SHARED, NACL_ABI_MAP_SHARED, -1, -1}
#define CHANNEL_MAP_PROT \
  {NACL_ABI_PROT_READ, NACL_ABI_PROT_WRITE, NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE, -1, -1}

/* premap given file to channel. return 0 if success */
int PremapChannel(struct NaClApp *nap, struct ChannelDesc* channel);

#endif /* PREMAP_H_ */
