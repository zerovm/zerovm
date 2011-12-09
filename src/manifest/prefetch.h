/*
 * preallocate network channel
 * note: reserved for a future zmq integration
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */

#ifndef PREFETCH_H_
#define PREFETCH_H_

/*
 * preallocate given network channel. return 0 if success, otherwise negative errcode
 */
int PrefetchChannel(struct NaClApp *nap, struct PreOpenedFileDesc* channel);

#endif /* PREFETCH_H_ */
