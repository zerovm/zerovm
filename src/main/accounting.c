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
 * d'b: changes for the new "simple" channels design
 * - new approach (actually it is the oldest one) to count i/o statistics:
 *   do it on demand, when Accounting() called
 * - "fast" and "regular" accounting should be replaced with 1 universal call
 */

#include <assert.h>
#include <time.h>
#include "src/loader/sel_ldr.h"
#include "src/main/accounting.h"
#include "src/main/manifest.h"

#define FMT "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %lu %lu"

static int64_t network_stats[LimitsNumber];
static int64_t local_stats[LimitsNumber];
static float user_time;
static float sys_time;

/* get I/O and CPU time */
static void SystemAccounting()
{
  FILE *f;
  int code;
  uint64_t t[4];
  char *path;
  float ticks = sysconf(_SC_CLK_TCK);

  /* get information from host */
  path = g_strdup_printf("/proc/%d/stat", getpid());
  f = fopen(path, "r");
  if(f == NULL) return;
  code = fscanf(f, FMT, &t[0], &t[1], &t[2], &t[3]);
  ZLOGIF(code != 4, "error %d occurred while reading '%s'", errno, path);

  /* calculate I/O and CPU time in seconds */
  sys_time = (t[1] + t[3]) / ticks;
  user_time = (t[0] + t[2]) / ticks;
  g_free(path);
  fclose(f);
}

/* i/o accounting */
static void IOAccounting(struct Manifest *manifest)
{
  int i;

  /*
   * if manifest is not yet initialized, than session is not started
   * and it is ok to show zeroes in account string
   */
  if(manifest == NULL) return;
  if(manifest->channels == NULL) return;

  for(i = 0; i < manifest->channels->len; ++i)
  {
    static int64_t *stats;
    struct ChannelDesc *channel = CH_CH(manifest, i);

    assert(channel != NULL);

    stats = IS_FILE(channel) ? local_stats : network_stats;
    stats[GetsLimit] += channel->counters[GetsLimit];
    stats[GetSizeLimit] += channel->counters[GetSizeLimit];
    stats[PutsLimit] += channel->counters[PutsLimit];
    stats[PutSizeLimit] += channel->counters[PutSizeLimit];
  }
}

/* returns string i/o statistics */
char *Accounting(struct Manifest *manifest)
{
  /* reset i/o stats */
  memset(network_stats, 0, sizeof network_stats);
  memset(local_stats, 0, sizeof network_stats);

  /* get accounted */
  IOAccounting(manifest);
  SystemAccounting();

  return g_strdup_printf("%.2f %.2f %ld %ld %ld %ld %ld %ld %ld %ld",
      sys_time,
      user_time,
      local_stats[GetsLimit], local_stats[GetSizeLimit],
      local_stats[PutsLimit], local_stats[PutSizeLimit],
      network_stats[GetsLimit], network_stats[GetSizeLimit],
      network_stats[PutsLimit], network_stats[PutSizeLimit]);
}
