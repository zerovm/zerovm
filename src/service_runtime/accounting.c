/*
 * accounting of the user resources: local i/o, network i/o, cpu and memory
 *
 * AccountingCtor(nap)
 * AccountingDtor(nap)
 * GetAccountingInfo()
 *
 *  Created on: Oct 14, 2012
 *      Author: d'b
 */
#include <assert.h>
#include <errno.h>
#include <glib.h>
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/accounting.h"
#include "src/manifest/manifest_setup.h"
#include "src/manifest/mount_channel.h"
#include "api/zvm.h"

/* accounting folder name. NULL if not available */
static char accounting[BIG_ENOUGH_STRING] = DEFAULT_ACCOUNTING;

/* populate "buf" with an extended accounting statistics, return string size */
static int ReadSystemAccounting(const struct NaClApp *nap, char *buf, int size)
{
  uint64_t user_time = 0;
  int64_t memory_size = 0;
  uint64_t sys_time = 0;
  uint64_t ticks;
  pid_t pid;
  int code;
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
      local_stats[GetsLimit], local_stats[GetSizeLimit], /* local channels input */
      local_stats[PutsLimit], local_stats[PutSizeLimit], /* local channels output */
      network_stats[GetsLimit], network_stats[GetSizeLimit], /* network channels input */
      network_stats[PutsLimit], network_stats[PutSizeLimit]);  /* network channels output */
}

/* initialize accounting */
void AccountingCtor(struct NaClApp *nap)
{
}

/* finalize accounting. return string with statistics */
void AccountingDtor(struct NaClApp *nap)
{
  int offset = 0;

  assert(nap != NULL);

  offset = ReadSystemAccounting(nap, accounting, BIG_ENOUGH_STRING);
  strncat(accounting, " ", 1);
  ++offset;
  GetChannelsAccounting(nap, accounting + offset, BIG_ENOUGH_STRING - offset);
}

/* return accounting string */
const char *GetAccountingInfo()
{
  return (const char*)accounting;
}
