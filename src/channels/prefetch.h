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

#include "src/channels/channel.h"
#include "src/main/manifest.h"

/* construct network channel */
void PrefetchChannelCtor(struct ChannelDesc *channel);

/* finalize and deallocate network channel */
void PrefetchChannelDtor(struct ChannelDesc *channel);

/*
 * send the data to the network channel
 * return number of sent bytes or negative error code
 */
int32_t SendData(struct ChannelDesc *channel, const char *buf, int32_t count);

/*
 * get the data from the network channel
 * return number of received bytes or negative error code or 0 when EOF
 */
int32_t FetchData(struct ChannelDesc *channel, char *buf, int32_t count);

#endif /* PREFETCH_H_ */
