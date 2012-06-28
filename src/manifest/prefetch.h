/*
 * preallocate network channel
 * todo(NETWORKING): update this file
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */

#ifndef PREFETCH_H_
#define PREFETCH_H_

/*
 * preallocate given network channel.
 * return 0 if success, otherwise negative errcode
 */
int PrefetchChannel(struct NaClApp *nap, struct ChannelDesc* channel);

#endif /* PREFETCH_H_ */
