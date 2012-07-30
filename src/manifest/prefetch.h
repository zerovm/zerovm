/*
 * preallocate network channel
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */

#ifndef PREFETCH_H_
#define PREFETCH_H_

#include "src/manifest/mount_channel.h"

#define NET_EOF -1 /* mark of the channel end */

/* construct network channel and connect/bind it to specified address */
int PrefetchChannelCtor(struct ChannelDesc* channel);

/* finalize and deallocate network channel resources */
int PrefetchChannelDtor(struct ChannelDesc* channel);

/*
 * fetch the data from the network channel
 * return number of received bytes or negative error code
 */
int32_t FetchMessage(struct ChannelDesc *channel, char *buf, int32_t count);

/*
 * send the data to the network channel
 * return number of sent bytes or negative error code
 */
int32_t SendMessage(struct ChannelDesc *channel, char *buf, int32_t count);

#endif /* PREFETCH_H_ */
