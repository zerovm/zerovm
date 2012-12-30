/*
 * zerovm log system. uses syslog to output messages.
 *
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
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <glib.h>
#include "src/main/tools.h"
#include "src/main/nacl_exit.h"
#include "src/main/zlog.h"

static int verbosity = 0;
static int zline = 0;
static const char *zfile = NULL;

/* initialize syslog */
void ZLogCtor(int v)
{
  verbosity = MAX(v, 0);
  openlog(ZLOG_NAME, ZLOG_OPTIONS, ZLOG_FACILITY);
}

/* close the log, reset verbosity level */
void ZLogDtor()
{
  verbosity = 0;
  closelog();
}

/*
 * store file and line information to the log tag
 * note: should only be used from ZLOG macro
 */
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

/*
 * append tag (if data is available) and put the message to syslog
 * note: should only be used from ZLOG macro
 */
void ZLog(int priority, char *fmt, ...)
{
  ZLO(priority > verbosity);
  assert(priority != LOG_FATAL);
}

/* if condition is true, log and continue */
void LogIf(int cond, char const *fmt, ...)
{
  ZLO(!cond);
}

/* if condition is true, log and abort */
void FailIf(int cond, int err, char const *fmt, ...)
{
  ZLO(!cond);
  SetExitState(msg);
  NaClExit(err);
}
