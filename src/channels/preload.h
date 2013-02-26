/*
 * preload given file to channel
 * note: with this class we make paging engine
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

#ifndef PRELOAD_H_
#define PRELOAD_H_

#include "src/channels/mount_channel.h"

#define CHANNEL_OPEN_FLAGS {O_RDONLY, O_RDWR | O_CREAT, O_RDWR | O_CREAT, -1, -1}
#define DEV_NULL "/dev/null"

/* disable space preallocation */
void PreloadAllocationDisable();

/*
 * preload given file to channel.
 * return 0 if success, otherwise negative errcode
 */
int PreloadChannelCtor(struct ChannelDesc* channel);

int PreloadChannelDtor(struct ChannelDesc* channel);

/* return the file source type or ChannelSourceTypeNumber */
enum ChannelSourceType GetChannelSource(const char *name);

#endif /* PRELOAD_H_ */
