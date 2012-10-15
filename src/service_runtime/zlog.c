/*
 * zerovm log system. uses syslog to output messages.
 *
 *  Created on: Oct 15, 2012
 *      Author: d'b
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include "src/platform/nacl_exit.h"
#include "src/service_runtime/zlog.h"

static int verbosity = LOWEST_VERBOSITY;

/* initialize syslog */
void ZLogCtor(int v)
{
  assert(v >= 0);

  verbosity = v + LOWEST_VERBOSITY;
  openlog(ZLOG_NAME, ZLOG_OPTIONS, ZLOG_PRIORITY);
}

/* close the log, reset verbosity level */
void ZLogDtor()
{
  verbosity = LOWEST_VERBOSITY;
  closelog();
}

/*
 * store file and line information to the log tag
 * note: if NULL given as "file" return previous tag
 * note: should only be used from ZLOG macro
 */
char *ZLogTag(const char *file, int line)
{
  static char msg[TAG_LIMIT];

  /* return empty tag */
  if(line == 0 && file != NULL) return "";

  /* todo(d'b): find replacement for snprintf() */
  if(file != NULL)
  {
    char *name = strrchr(file, '/');
    name = name == NULL ? (char*)file : name + 1;
    sprintf(msg, TAG_FORMAT, name, line);
  }
  return msg;
}

/*
 * todo(d'b): ZLog(), FailIf() and LogIf consist almost of the same
 * code find the solution how to reduce/remove the code doubling
 */

/*
 * append tag and put given message to syslog
 * note: should only be used from ZLOG macro
 */
void ZLog(int priority, char *fmt, ...)
{
  char msg[LOG_MSG_LIMIT];
  va_list ap;

  /* skip insignificant messages */
  if(priority >= verbosity) return;

  va_start(ap, fmt);
  vsprintf(msg, fmt, ap);
  va_end(ap);
  syslog(ZLOG_PRIORITY, "%s %s", ZLogTag(NULL, 0), msg);

  /* abort if fail occurred */
  if(priority == LOG_FATAL) NaClAbort();
}

/* if condition is true, log and abort */
void FailIf(int cond, char const *fmt, ...)
{
  char msg[LOG_MSG_LIMIT];
  va_list ap;

  /* skip false conditions */
  if(!cond) return;

  va_start(ap, fmt);
  vsprintf(msg, fmt, ap);
  va_end(ap);
  syslog(ZLOG_PRIORITY, "%s %s", ZLogTag(NULL, 0), msg);
  NaClAbort();
}

/* if condition is true, log and continue */
void LogIf(int cond, char const *fmt, ...)
{
  char msg[LOG_MSG_LIMIT];
  va_list ap;

  /* skip false conditions */
  if(!cond) return;

  va_start(ap, fmt);
  vsprintf(msg, fmt, ap);
  va_end(ap);
  syslog(ZLOG_PRIORITY, "%s %s", ZLogTag(NULL, 0), msg);
}
