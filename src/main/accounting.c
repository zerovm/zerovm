/*
 * accounting of the user resources: local i/o, network i/o, cpu and memory
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

#include <assert.h>
#include <errno.h>
#include <glib.h>
#include "src/loader/sel_ldr.h"
#include "src/main/accounting.h"
#include "src/main/manifest_setup.h"
#include "src/channels/mount_channel.h"

/* accounting folder name */
static char accounting[BIG_ENOUGH_STRING] = DEFAULT_ACCOUNTING;

/* populate "buf" with an extended accounting statistics, return string size */
static int ReadSystemAccounting(const struct NaClApp *nap, char *buf, int size)
{
  uint64_t user_time = 0;
  int64_t memory_size = 0;
  uint64_t sys_time = 0;
  uint64_t ticks;
  pid_t pid;
  FILE *f;
  char path[BIG_ENOUGH_STRING];

  assert(nap != NULL);
  assert(buf != NULL);
  assert(nap->system_manifest != NULL);
  assert(size > 0);

  /* get time information */
  ticks = sysconf(_SC_CLK_TCK);
  pid = getpid();
  g_snprintf(path, BIG_ENOUGH_STRING, "/proc/%d/stat", pid);

  f = fopen(path, "r");
  if(f != NULL)
  {
    int code;
    code = fscanf(f, STAT_FMT, &user_time, &sys_time);
    ZLOGIF(code != 2, "error %d occured while reading '%s'", errno, path);
    fclose(f);
  }

  /* get memory information */
  memory_size = nap->heap_end + nap->stack_size;

  /* construct and return the result */
  return g_snprintf(buf, size, "%.2f %.2f %ld %ld",
      (float)sys_time / ticks, (float)user_time / ticks, memory_size, (int64_t)0);
}

/* get internal i/o information from channels counters */
static int GetChannelsAccounting(const struct NaClApp *nap, char *buf, int size)
{
  int64_t network_stats[IOLimitsCount] = {0};
  int64_t local_stats[IOLimitsCount] = {0};
  int i;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert(buf != NULL);
  assert(size != 0);

  /* gather all channels statistics */
  for(i = 0; i < nap->system_manifest->channels_count; ++i)
  {
    struct ChannelDesc *channel = &nap->system_manifest->channels[i];
    int64_t *stats = NULL;
    int j;

    /* select proper stats array */
    switch(channel->source)
    {
      case ChannelRegular:
      case ChannelCharacter:
      case ChannelFIFO:
        stats = local_stats;
        break;
      case ChannelTCP:
        stats = network_stats;
        break;
      default:
        ZLOG(LOG_ERR, "'%s': source type '%s' not supported",
            channel->name, StringizeChannelSourceType(channel->source));
        buf = DEFAULT_ACCOUNTING;
        return sizeof DEFAULT_ACCOUNTING;
    }

    for(j = 0; j < IOLimitsCount; ++j)
      stats[j] += channel->counters[j];
  }

  /* construct the accounting statistics string */
  return g_snprintf(buf, size, "%ld %ld %ld %ld %ld %ld %ld %ld",
      local_stats[GetsLimit], local_stats[GetSizeLimit],
      local_stats[PutsLimit], local_stats[PutSizeLimit],
      network_stats[GetsLimit], network_stats[GetSizeLimit],
      network_stats[PutsLimit], network_stats[PutSizeLimit]);
}

void AccountingCtor(const struct NaClApp *nap)
{
}

void AccountingDtor(const struct NaClApp *nap)
{
  int offset = 0;

  assert(nap != NULL);

  offset = ReadSystemAccounting(nap, accounting, BIG_ENOUGH_STRING);
  strncat(accounting, " ", 1);
  ++offset;
  GetChannelsAccounting(nap, accounting + offset, BIG_ENOUGH_STRING - offset);
}

const char *GetAccountingInfo()
{
  return (const char*)accounting;
}
