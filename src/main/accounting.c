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

#include <assert.h>
#include "src/loader/sel_ldr.h"
#include "src/main/accounting.h"
#include "src/channels/channel.h"

/* accounting folder name */
static char accounting[BIG_ENOUGH_STRING] = DEFAULT_ACCOUNTING;

/* populate "buf" with an extended accounting statistics, return string size */
static int ReadSystemAccounting(const struct NaClApp *nap, char *buf, int size)
{
  uint64_t user_time = 0;
  uint64_t sys_time = 0;
  uint64_t c_user_time = 0; /* network time */
  uint64_t c_sys_time = 0; /* network time */
  int64_t memory_size = 0;
  uint64_t ticks;
  pid_t pid;
  FILE *f;
  char path[BIG_ENOUGH_STRING];

  assert(buf != NULL);
  assert(size > 0);

  /* get time information */
  ticks = sysconf(_SC_CLK_TCK);
  pid = getpid();
  g_snprintf(path, BIG_ENOUGH_STRING, "/proc/%d/stat", pid);

  f = fopen(path, "r");
  if(f != NULL)
  {
    int code;
    code = fscanf(f, STAT_FMT, &user_time, &sys_time, &c_user_time, &c_sys_time);
    ZLOGIF(code != 4, "error %d occured while reading '%s'", errno, path);

    /* combine user times and system times */
    user_time += c_user_time;
    sys_time += c_sys_time;
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
  int64_t network_stats[LimitsNumber] = {0};
  int64_t local_stats[LimitsNumber] = {0};
  int i;

  assert(buf != NULL);
  assert(size != 0);

  /* gather all channels statistics */
  for(i = 0; i < nap->manifest->channels->len; ++i)
  {
    struct ChannelDesc *channel = CH_CH(nap->manifest, i);
    int64_t *stats = NULL;
    int j;

    /*
     * select proper array. channels with mixed source types
     * cannot be properly accounted and will be accounted by
     * the 1st source type
     */
    switch(CH_PROTO(channel, 0))
    {
      case ProtoRegular:
      case ProtoCharacter:
      case ProtoFIFO:
        stats = local_stats;
        break;
      case ProtoTCP:
        stats = network_stats;
        break;
      default:
        ZLOG(LOG_ERR, "%s has invalid source type %s", channel->alias);
        buf = DEFAULT_ACCOUNTING;
        return sizeof DEFAULT_ACCOUNTING;
    }

    for(j = 0; j < LimitsNumber; ++j)
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

  if(nap->manifest == NULL || nap->manifest->channels == NULL) return;

  offset = ReadSystemAccounting(nap, accounting, BIG_ENOUGH_STRING);
  strncat(accounting, " ", 1);
  ++offset;
  GetChannelsAccounting(nap, accounting + offset, BIG_ENOUGH_STRING - offset);
}

const char *GetAccountingInfo()
{
  return (const char*)accounting;
}
