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
#include "src/main/config.h"
#include "src/main/zlog.h"
#include "src/main/accounting.h"
#include "src/main/etag.h"
#include "src/loader/usermap.h"
#include "src/main/report.h"

#ifdef DEBUG
#define REPORT_VALIDATOR "validator state = "
#define REPORT_DAEMON "daemon = "
#define REPORT_RETCODE "user return code = "

/* DEPRECATED. API version 1. */
#ifndef REMOVE_DEPRECATED
#define REPORT_ETAG "etag(s) = "
#else
#define REPORT_ETAG "memtag = "
#endif

#define REPORT_ACCOUNTING "accounting = "
#define REPORT_STATE "exit state = "
#define REPORT_CMD ReportSetupPtr()->cmd
#else
#define REPORT_VALIDATOR ""
#define REPORT_DAEMON ""
#define REPORT_RETCODE ""
#define REPORT_ETAG ""
#define REPORT_ACCOUNTING ""
#define REPORT_STATE ""
#define REPORT_CMD ""
#endif

struct ReportSetup *ReportSetupPtr()
{
  static struct ReportSetup report;
  return &report;
}

/* calculate user memory tag, and return pointer to it */
static char *GetMemoryDigest(struct Manifest *manifest)
{
  int i = 0;
  int j;
  int size;
  static char digest[TAG_DIGEST_SIZE + 1];
  uint8_t *map = GetUserMap();

  if(manifest == NULL || manifest->mem_tag == NULL) return "disabled";

  while(i < FOURGIG / NACL_MAP_PAGESIZE)
  {
    /* count readable pages */
    for(j = i; j < FOURGIG / NACL_MAP_PAGESIZE; ++j)
      if((map[j] & PROT_READ) == 0) break;

    size = j - i;
    if(size)
    {
      TagUpdate(manifest->mem_tag,
          (void*)MEM_START + i * NACL_MAP_PAGESIZE, size * NACL_MAP_PAGESIZE);
      i += j;
    }
    else
      ++i;
  }

  TagDigest(manifest->mem_tag, digest);
  return digest;
}

/* output report */
/* TODO(d'b): rework "-t" and update the function */
static void OutputReport(char *r)
{
  char *p = NULL;
  int size = strlen(r);

#define REP(p) ZLOGIF(write(ReportSetupPtr()->handle, p, size) != size, \
  "report write error %d: %s", errno, strerror(errno))

  switch(ReportSetupPtr()->mode)
  {
    case 3: /* unix socket */
      p = g_strdup_printf("0x%06x%s", size, r);
      size = strlen(p);
      REP(p);
      g_free(p);
      break;
    case 0: /* stdout */
      ReportSetupPtr()->handle = STDOUT_FILENO;
      REP(r);
      break;
    case 1: /* syslog */
      ZLOGS(LOG_ERROR, "%s", r);
      break;
    default:
      ZLOG(LOG_ERROR, "invalid report mode %d", ReportSetupPtr()->mode);
      break;
  }
#undef REP
}

/* part of report class dtor */
#define REP g_string_append_printf
void Report(struct Manifest *manifest)
{
  GString *r = g_string_sized_new(BIG_ENOUGH_STRING);
  char *eol = ReportSetupPtr()->mode == 1 ? "; " : "\n";

  /* report validator state and user return code */
  REP(r, "%s%d%s", REPORT_VALIDATOR, ReportSetupPtr()->validation_state, eol);
  REP(r, "%s%d%s", REPORT_DAEMON, ReportSetupPtr()->daemon_pid, eol);
  REP(r, "%s%lu%s", REPORT_RETCODE, ReportSetupPtr()->user_code, eol);
  REP(r, "%s%s%s", REPORT_ETAG, GetMemoryDigest(manifest), eol);
  REP(r, "%s%s%s", REPORT_ACCOUNTING, Accounting(manifest), eol);
  REP(r, "%s%s%s", REPORT_STATE, ReportSetupPtr()->zvm_state == NULL
      ? UNKNOWN_STATE : ReportSetupPtr()->zvm_state, eol);
  REP(r, "%s%s", REPORT_CMD, eol);
  OutputReport(r->str);

  g_string_free(r, TRUE);
}
