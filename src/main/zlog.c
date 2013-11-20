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
#include "src/main/report.h"
#include "src/main/zlog.h"

#define ZLOG_NAME "ZeroVM"
#define ZLOG_OPTIONS (LOG_PID)
#define ZLOG_FACILITY LOG_USER
#define ZLOG_PRIORITY LOG_ERROR
#define TAG_FORMAT "%s %d: "
#define LOG_MSG_LIMIT 0x1000

static int verbosity = 0;
static int zline = 0;
static const char *zfile = NULL;

void ZLogCtor(int v)
{
  verbosity = MAX(v, 0);
  openlog(ZLOG_NAME, ZLOG_OPTIONS, ZLOG_FACILITY);
}

void ZLogDtor()
{
  verbosity = 0;
  closelog();
}

void ZLogTag(const char *file, int line)
{
  zline = line;
  zfile = file;
}

#define ZLO(cond) \
  char msg[LOG_MSG_LIMIT];\
  int offset = 0;\
  va_list ap;\
\
  if(cond) return;\
\
  /* construct log message */\
  va_start(ap, fmt);\
  if(zfile != NULL)\
    offset = g_snprintf(msg, LOG_MSG_LIMIT, TAG_FORMAT, zfile, zline);\
  g_vsnprintf(msg + offset, LOG_MSG_LIMIT - offset, fmt, ap);\
  va_end(ap);\
\
  /* log the message */\
  syslog(ZLOG_PRIORITY, "%s", msg);

void ZLog(int priority, char *fmt, ...)
{
  ZLO(priority > verbosity);
  assert(priority != LOG_FATAL);
}

void LogIf(int cond, char const *fmt, ...)
{
  ZLO(!cond);
}

void FailIf(int cond, int err, char const *fmt, ...)
{
  ZLO(!cond);
  SetExitState(msg);
  ReportDtor(err);
}
