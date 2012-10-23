/*
 * zerovm log system. uses syslog to output messages.
 *
 *  Created on: Oct 15, 2012
 *      Author: d'b
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "src/service_runtime/tools.h"
#include "src/platform/nacl_exit.h"
#include "src/service_runtime/zlog.h"

static int verbosity = 0;
static int zline = 0;
static const char *zfile = NULL;

/* initialize syslog */
void ZLogCtor(int v)
{
  verbosity = MAX(v, 0);
  openlog(ZLOG_NAME, ZLOG_OPTIONS, ZLOG_PRIORITY);
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
 * todo(d'b): find replacement for snprintf()
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
    offset = sprintf(msg, TAG_FORMAT, zfile, zline);\
  vsprintf(msg + offset, fmt, ap);\
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
  if(priority == LOG_FATAL) NaClAbort();
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
