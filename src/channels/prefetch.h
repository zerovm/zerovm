/*
 * preallocate network channel
 *
 * Copyright (c) 2012, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PREFETCH_H_
#define PREFETCH_H_

#include "src/channels/mount_channel.h"

/* return ChannelSourceType for network channels */
enum ChannelSourceType GetChannelProtocol(const char *url);

/* construct network channel and connect/bind it to specified address */
int PrefetchChannelCtor(struct ChannelDesc* channel);

/*
 * finalize and deallocate network channel resources
 * todo(d'b): rewrite the code after zmq_term will be fixed
 */
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
int32_t SendMessage(struct ChannelDesc *channel, const char *buf, int32_t count);

/*
 * if name service is available then go through all available
 * channels, pick the network ones and connect them
 * note: if no name service is available quietly return - all channels
 * are already bound and connected due 1st pass
 */
void KickPrefetchChannels(struct NaClApp *nap);

#endif /* PREFETCH_H_ */
