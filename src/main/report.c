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
static int validation_state = 2;
static const char *zvm_state = UNKNOWN_STATE;
static GString *digests = NULL; /* cumulative etags */
static int hide_report = 0; /* if not 0 report to syslog */
static char *debug_string = NULL;

void HideReport()
{
  hide_report = 1;
}

void SetExitState(const char *state)
{
  assert(state != NULL);
  zvm_state = state;
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

void SetValidationState(int state)
{
  validation_state = state;
}

void SetDebugString(const char *s)
{
  debug_string = g_strdup(s);
}

char *GetDebugString()
{
  return debug_string;
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
#define REPORT g_string_append_printf
static void Report(struct NaClApp *nap)
{
  GString *r = g_string_sized_new(BIG_ENOUGH_STRING);
  char *eol = hide_report ? "; " : "\n";

  /* report validator state and user return code */
  REPORT(r, "%s%d%s", REPORT_VALIDATOR, validation_state, eol);
  REPORT(r, "%s%d%s", REPORT_RETCODE, user_code, eol);

  /* add memory digest to cumulative digests if asked */
  if(nap != NULL && nap->manifest != NULL)
    if(nap->manifest->mem_tag != NULL)
      ReportTag(STDRAM, GetMemoryDigest(nap));

  /* report tags digests and remove ending " " if exist */
  REPORT(r, "%s", REPORT_ETAG);
  REPORT(r, "%s", digests->len == 0
      ? TAG_ENGINE_DISABLED : digests->str);
  g_string_truncate(r, r->len - 1);

  /* report accounting and session message */
  REPORT(r, "%s%s%s%s", eol, REPORT_ACCOUNTING, GetAccountingInfo(), eol);
  REPORT(r, "%s%s%s", REPORT_STATE, zvm_state, eol);
  REPORT(r, "%s%s", REPORT_DEBUG, eol);

  /* output report */
  if(hide_report)
    ZLOGS(LOG_ERROR, "%s", r->str);
  else
    ZLOGIF(write(STDOUT_FILENO, r->str, r->len) != r->len,
        "report write error %d: %s", errno, strerror(errno));

  g_string_free(r, TRUE);
  g_string_free(digests, TRUE);
  g_free(debug_string);
}

void ReportDtor(int zvm_ret)
{
  SetExitCode(zvm_ret);

  /* broken session */
  if(zvm_code != 0)
  {
    ZLOGS(LOG_ERROR, "SESSION %d FAILED WITH ERROR %d: %s",
        gnap->manifest == NULL ? 0 : gnap->manifest->node,
        zvm_code, strerror(zvm_code));
    FinalDump(gnap);
  }

  ChannelsDtor(gnap->manifest);
  AccountingDtor(gnap);
  Report(gnap);
  NaClAppDtor(gnap); /* free user space and globals */
  ManifestDtor(gnap->manifest); /* dispose manifest and channels */
  FreeDispatchThunk();
  ZLogDtor();

  _exit(zvm_code);
}
