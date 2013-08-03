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
#include <sys/mman.h>
#include "src/main/report.h"
#include "src/platform/signal.h"
#include "src/main/accounting.h"
#include "src/channels/channel.h"

static int zvm_code = 0;
static int user_code = 0;
static int validation_state = 0;
static const char *zvm_state = UNKNOWN_STATE;
static GString *digests = NULL; /* cumulative etags */

void SetExitState(const char *state)
{
  assert(state != NULL);
  zvm_state = state;
}

const char *GetExitState()
{
  assert(zvm_state != NULL);
  return zvm_state;
}

void SetExitCode(int code)
{
  /* only the 1st error matters */
  if(zvm_code == 0) zvm_code = code;
}

int GetExitCode()
{
  return zvm_code;
}

void SetUserCode(int code)
{
  user_code = code;
}

int GetUserCode()
{
  return user_code;
}

void SetValidationState(int state)
{
  validation_state = state;
}

int GetValidationState()
{
  return validation_state;
}

/* manage special signals. note: may change zvm_state */
/* todo(d'b): remove this patch after reworking zvm signals */
static void SpecSignals()
{
  if(strstr(zvm_state, "Signal 14") != NULL)
    SetExitState("session timeout");
  else if(strstr(zvm_state, "Signal 25") != NULL)
    SetExitState("disk quota exceeded");
}

/* log user memory map */
static void LogMemMap(struct NaClApp *nap, int verbosity)
{
  int i;

  ZLOGS(verbosity, "user memory map (in pages):");

  for(i = 0; i < MemMapSize; ++i)
  {
    ZLOGS(verbosity, "%s: address = 0x%08x, size = 0x%08x, protection = x",
        nap->mem_map[i].name, (uint32_t) nap->mem_map[i].start,
        (uint32_t) nap->mem_map[i].size, nap->mem_map[i].prot);
  }
}

static void FinalDump(struct NaClApp *nap)
{
  ZLOGS(LOG_INSANE, "exiting -- printing NaClApp details");

  /* NULL can be used because syslog used for nacl log */
  PrintAppDetails(nap, (struct Gio *) NULL, LOG_INSANE);
  LogMemMap(nap, LOG_INSANE);

  SignalHandlerFini();
}

void ReportTag(char *name, void *tag)
{
  char digest[TAG_DIGEST_SIZE + 1];

  assert(name != NULL);
  if(tag == NULL) return;

  TagDigest(tag, digest);
  g_string_append_printf(digests, "%s %s ", name, digest);
}

/* calculate user memory tag, and return pointer to it */
static void *GetMemoryDigest(struct NaClApp *nap)
{
  int i;

  assert(nap != NULL);

  /* calculate overall memory tag */
  for(i = 0; i < MemMapSize; ++i)
  {
    uintptr_t addr = nap->mem_map[i].start;
    int64_t size = nap->mem_map[i].size;

    /* update user_etag skipping inaccessible pages */
    if(nap->mem_map[i].prot & PROT_READ)
      TagUpdate(nap->manifest->mem_tag, (const char*)addr, size);
  }

  return nap->manifest->mem_tag;
}

void ReportCtor()
{
  digests = g_string_sized_new(BIG_ENOUGH_STRING);
}

/* part of report class dtor */
static void Report(struct NaClApp *nap)
{
  GString *report = g_string_sized_new(BIG_ENOUGH_STRING);

  /* report validator state and user return code */
  g_string_append_printf(report, "%s%d\n", REPORT_VALIDATOR, GetValidationState());
  g_string_append_printf(report, "%s%d\n", REPORT_RETCODE, GetUserCode());

  /* add memory digest to cumulative digests if asked */
  if(nap != NULL && nap->manifest != NULL)
    if(nap->manifest->mem_tag != NULL)
      ReportTag(STDRAM, GetMemoryDigest(nap));

  /* report tags digests and remove ending " " if exist */
  g_string_append_printf(report, "%s", REPORT_ETAG);
  g_string_append_printf(report, "%s", digests->len == 0
      ? TAG_ENGINE_DISABLED : digests->str);
  g_strchomp(report->str);

  /* report accounting and session message */
  g_string_append_printf(report, "\n%s%s\n", REPORT_ACCOUNTING, GetAccountingInfo());
  g_string_append_printf(report, "%s%s\n", REPORT_STATE, GetExitState());

  /* output report and free resources */
  ZLOGIF(write(STDOUT_FILENO, report->str, report->len) != report->len,
      "report write error %d: %s", errno, strerror(errno));
  g_string_free(report, TRUE);
  g_string_free(digests, TRUE);
}

void ReportDtor(int zvm_ret)
{
  SetExitCode(zvm_ret);

  /*
   * patch to show special messages instead of signals 24, 25
   * and do not calculate etag if session failed
   */
  if(zvm_code != 0)
  {
    SpecSignals();
    ZLOGS(LOG_ERROR, "SESSION %d FAILED WITH ERROR %d: %s",
        gnap->manifest == NULL ? 0 : gnap->manifest->node,
        zvm_code, strerror(zvm_code));
  }

  if(zvm_code != 0) FinalDump(gnap);

  AccountingDtor(gnap);
  ChannelsDtor(gnap->manifest);
  Report(gnap);
  NaClAppDtor(gnap); /* free user space and globals */
  ManifestDtor(gnap->manifest); /* dispose manifest and channels */
  FreeDispatchThunk();
  ZLogDtor();

  _exit(zvm_code);
}
