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
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include "src/main/zlog.h"
#include "src/main/accounting.h"
#include "src/channels/channel.h"

#define ACCOUNTING_SIZE 128
#define FMT "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %lu %lu"
#define INVALID_TIME "0.0 0.0"
#define INVALID_IO "0 0 0 0 0 0 0 0"

/* get I/O and CPU time */
static char *SystemAccounting()
{
  FILE *f;
  int code;
  uint64_t t[4];
  char path[BIG_ENOUGH_STRING];
  float ticks = sysconf(_SC_CLK_TCK);

  /* get information from host */
  g_snprintf(path, BIG_ENOUGH_STRING, "/proc/%d/stat", getpid());
  f = fopen(path, "r");
  if(f == NULL)
  {
    ZLOG(LOG_ERROR, "cannot open stat");
    return g_strdup(INVALID_TIME);
  }

  code = fscanf(f, FMT, &t[0], &t[1], &t[2], &t[3]);
  fclose(f);
  if(code != 4)
  {
    ZLOG(LOG_ERROR, "cannot read stat");
    return g_strdup(INVALID_TIME);
  }

  return g_strdup_printf("%.2f %.2f",
      (t[1] + t[3]) / ticks, (t[0] + t[2]) / ticks);
}

/* i/o accounting */
static char *IOAccounting(struct Manifest *manifest)
{
  int i;
  int64_t net[LimitsNumber] = {0};
  int64_t loc[LimitsNumber] = {0};

  /*
   * if manifest is not yet initialized, than session is not started
   * and it is ok to show zeroes in the accounting string
   */
  if(manifest == NULL || manifest->channels == NULL)
    return g_strdup(INVALID_IO);

  for(i = 0; i < manifest->channels->len; ++i)
  {
    static int64_t *io;
    struct ChannelDesc *channel = CH_CH(manifest, i);

    assert(channel != NULL);

    io = IS_FILE(channel) ? loc : net;
    io[GetsLimit] += channel->counters[GetsLimit];
    io[GetSizeLimit] += channel->counters[GetSizeLimit];
    io[PutsLimit] += channel->counters[PutsLimit];
    io[PutSizeLimit] += channel->counters[PutSizeLimit];
  }

  return g_strdup_printf("%ld %ld %ld %ld %ld %ld %ld %ld",
      loc[GetsLimit], loc[GetSizeLimit],
      loc[PutsLimit], loc[PutSizeLimit],
      net[GetsLimit], net[GetSizeLimit],
      net[PutsLimit], net[PutSizeLimit]);
}

/* returns string i/o statistics */
char *Accounting(struct Manifest *manifest)
{
  static char result[ACCOUNTING_SIZE];
  char *time = SystemAccounting();
  char *io = IOAccounting(manifest);
  
  g_snprintf(result, ACCOUNTING_SIZE, "%s %s", time, io);
  g_free(time);
  g_free(io);
  return result;
}
