/*
 * channels constructor / destructor API
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

#ifndef CHANNEL_H_
#define CHANNEL_H_

#include <zmq.h>
#include "api/zvm.h"
#include "src/main/etag.h"
#include "src/main/manifest.h"

EXTERN_C_BEGIN

/* zerovm channels names */
#define STDIN "/dev/stdin"
#define STDOUT "/dev/stdout"
#define STDERR "/dev/stderr"
#define STDRAM "/dev/memory"

#define IS_NETWORK(proto) (proto < ProtoRegular)
#define IS_FILE(proto) (!IS_NETWORK(proto))
#define IS_IPHOST(flags) (flags & 1)

/* CH_RW_TYPE returns 0..3 */
#define IS_NIL(channel) (CH_RW_TYPE(channel) == 0)
#define IS_RO(channel) (CH_RW_TYPE(channel) == 1)
#define IS_WO(channel) (CH_RW_TYPE(channel) == 2)
#define IS_RW(channel) (CH_RW_TYPE(channel) == 3)
#define CH_RW_TYPE(channel) \
    (((channel)->limits[GetsLimit] && (channel)->limits[GetSizeLimit]) \
    | ((channel)->limits[PutsLimit] && (channel)->limits[PutSizeLimit]) << 1)

#define CH_CH(manifest, n) ((struct ChannelDesc*)g_ptr_array_index(manifest->channels, n))
#define CH_SOURCE(channel, n) ((struct Connection*)g_ptr_array_index(channel->source, n))
#define CH_HANDLE(channel, n) (CH_SOURCE(channel, n))->handle
#define CH_PROTO(channel, n) (CH_SOURCE(channel, n))->protocol
#define CH_NAME(channel, n) ((struct File*)CH_SOURCE(channel, n))->name
#define CH_HOST(channel, n) (CH_SOURCE(channel, n))->host
#define CH_PORT(channel, n) (CH_SOURCE(channel, n))->port
#define CH_FLAGS(channel, n) (CH_SOURCE(channel, n))->flags

/* todo: rewrite it or remove it */
#define CH_SEQ_READABLE(channel) (channel->type == 0 || channel->type == 2)
#define CH_SEQ_WRITEABLE(channel) (channel->type == 0 || channel->type == 1)
#define CH_RND_READABLE(channel) (channel->type == 1 || channel->type == 3)
#define CH_RND_WRITEABLE(channel) (channel->type == 2 || channel->type == 3)

/* construct all channels, initialize it and update system_manifest */
void ChannelsCtor(struct Manifest *manifest);

/* free channels resources */
void ChannelsDtor(struct Manifest *manifest);

/* read channel data through multiple sources */
int32_t ChannelRead(struct ChannelDesc *channel,
    char *buffer, size_t size, off_t offset);

/* write channel data through multiple sources */
int32_t ChannelWrite(struct ChannelDesc *channel,
    const char *buffer, size_t size, off_t offset);

EXTERN_C_END

#endif /* CHANNEL_H_ */
