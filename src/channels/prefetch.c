/*
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

/*
 * wrapper for zvm_pipes
 * TODO(d'b): should be removed after re-factoring channels class
 */
#include <zvm_zpipes.h>
#include "src/main/zlog.h"
#include "src/channels/prefetch.h"

int32_t FetchData(struct ChannelDesc *channel, char *buf, int32_t size)
{
  ZLOGS(LOG_INSANE, "FetchData(%s, %p, %d)", channel->alias, buf, size);
  return zvm_pipe_read(channel->handle, buf, size);
}

int32_t SendData(struct ChannelDesc *channel, const char *buf, int32_t size)
{
  ZLOGS(LOG_INSANE, "SendData(%s, %p, %d)", channel->alias, buf, size);
  return zvm_pipe_write(channel->handle, (void*)buf, size);
}

void PrefetchChannelCtor(struct ChannelDesc *channel)
{
  ZLOGS(LOG_DEBUG, "PrefetchChannelCtor(%s)", channel->alias);
  channel->handle = zvm_pipe_open(channel->name);
  ZLOGFAIL(channel->handle == NULL, EIO, "cannot open %s", channel->name);
  channel->size = 0;
  ZLOGS(LOG_DEBUG, "%s opened", channel->alias);
}

void PrefetchChannelDtor(struct ChannelDesc *channel)
{
  int i;

  ZLOGS(LOG_DEBUG, "PrefetchChannelDtor(%s)", channel->alias);
  i = zvm_pipe_close(channel->handle);
  ZLOGS(LOG_DEBUG, "%s", channel->alias, i ? "closed" : "failed to close");
}
