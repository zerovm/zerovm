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
#include "src/main/manifest.h"

/* accounting folder name */
static char accounting[BIG_ENOUGH_STRING] = DEFAULT_ACCOUNTING;
static int64_t network_stats[LimitsNumber] = {0};
static int64_t local_stats[LimitsNumber] = {0};

/* count i/o statistics */
static void CountBytes(struct Connection *c, int size, int index)
{
  int64_t *acc;

  assert(c != NULL);
  assert(size >= 0);

  /* update statistics */
  acc = IS_FILE(c) ? local_stats : network_stats;
  acc[index + 1] += size;
  ++acc[index];
}

void CountGet(struct Connection *c, int size)
{
  CountBytes(c, size, GetsLimit);
}

void CountPut(struct Connection *c, int size)
{
  CountBytes(c, size, PutsLimit);
}

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
      (float)sys_time / ticks, (float)user_time / ticks, memory_size, 0L);
}

/* prepare string with i/o information */
static int GetChannelsAccounting(char *buf, int size)
{
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
  GetChannelsAccounting(accounting + offset, BIG_ENOUGH_STRING - offset);
}

const char *GetAccountingInfo()
{
  return (const char*)accounting;
}
