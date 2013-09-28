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
#include <time.h>
#include "src/loader/sel_ldr.h"
#include "src/main/accounting.h"
#include "src/main/manifest.h"

#define FMT "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %lu %lu"

static int64_t network_stats[LimitsNumber] = {0};
static int64_t local_stats[LimitsNumber] = {0};
static float user_time = 0;
static float sys_time = 0;

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

/* get I/O and CPU time */
static void SystemAccounting()
{
  char path[BIG_ENOUGH_STRING];
  uint64_t utime = 0;
  uint64_t stime = 0;
  uint64_t nutime = 0; /* network time */
  uint64_t nstime = 0; /* network time */
  float ticks;
  pid_t pid;
  int code;
  FILE *f;

  /* get time information */
  ticks = sysconf(_SC_CLK_TCK);
  pid = getpid();
  g_snprintf(path, BIG_ENOUGH_STRING, "/proc/%d/stat", pid);

  f = fopen(path, "r");
  if(f == NULL) return;
  code = fscanf(f, FMT, &utime, &stime, &nutime, &nstime);
  ZLOGIF(code != 4, "error %d occurred while reading '%s'", errno, path);

  /* I/O and CPU time */
  sys_time = (stime + nstime) / ticks;
  user_time = (utime + nutime) / ticks;
  fclose(f);
}

/* returns string i/o statistics */
static char *Accounting(int fast)
{
  return g_strdup_printf("%.2f %.2f %ld %ld %ld %ld %ld %ld %ld %ld",
      fast ? 0 : sys_time /* TODO(d'b): put I/O time instead of 0 */,
      fast ? clock() / (float)CLOCKS_PER_SEC : user_time,
      local_stats[GetsLimit], local_stats[GetSizeLimit],
      local_stats[PutsLimit], local_stats[PutSizeLimit],
      network_stats[GetsLimit], network_stats[GetSizeLimit],
      network_stats[PutsLimit], network_stats[PutSizeLimit]);
}

char *FastAccounting()
{
  return Accounting(1);
}

char *FinalAccounting()
{
  assert(user_time == 0);
  assert(sys_time == 0);

  SystemAccounting();
  return Accounting(0);
}
