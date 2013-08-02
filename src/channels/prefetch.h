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

/* get control_digest for the last read RO source reached eof */
char *GetControlDigest();

/* prepare network context */
void NetCtor(const struct Manifest *manifest);

/* deallocate network context */
void NetDtor(struct Manifest *manifest);

/* construct network channel and connect/bind it to specified address */
void PrefetchChannelCtor(struct ChannelDesc *channel, int n);

/*
 * finalize and deallocate network channel resources
 * todo(d'b): rewrite the code after zmq_term will be fixed
 */
void PrefetchChannelDtor(struct ChannelDesc *channel, int n);

/*
 * fetch the data from the network channel
 * return number of received bytes or negative error code
 */
int32_t FetchMessage(struct ChannelDesc *channel, int n, char *buf, int32_t count);

/*
 * send the data to the network channel
 * return number of sent bytes or negative error code
 */
int32_t SendMessage(struct ChannelDesc *channel, int n, const char *buf, int32_t count);

#endif /* PREFETCH_H_ */
