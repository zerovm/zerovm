/*
 * preload given file to channel
 * note: with this class we make paging engine
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */

#ifndef PRELOAD_H_
#define PRELOAD_H_

#include "src/channels/mount_channel.h"

#define CHANNEL_OPEN_FLAGS {O_RDONLY, O_RDWR | O_CREAT, O_RDWR | O_CREAT, -1, -1}
#define DEV_NULL "/dev/null"

/*
 * preload given file to channel.
 * return 0 if success, otherwise negative errcode
 */
int PreloadChannelCtor(struct ChannelDesc* channel);

/* todo(d'b): under construction */
int PreloadChannelDtor(struct ChannelDesc* channel);

/* return the file source type or ChannelSourceTypeNumber */
enum ChannelSourceType GetChannelSource(const char *name);

#endif /* PRELOAD_H_ */
