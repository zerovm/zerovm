/*
 * zerovm log system. uses syslog to output messages.
 *
 *  Created on: Oct 15, 2012
 *      Author: d'b
 */

#ifndef ZLOG_H_
#define ZLOG_H_

#include <syslog.h>
#include "src/include/nacl_base.h"

EXTERN_C_BEGIN

#define ZLOG ZLogTag(__FILE__, __LINE__), ZLog
#define ZLOGS ZLogTag("", 0), ZLog
#define LOG_MSG_LIMIT 0x1000
#define TAG_FORMAT "%s %d:"
#define FILENAME_LIMIT 0x100
#define FILELINES_LIMIT 10
#define TAG_LIMIT FILENAME_LIMIT + FILELINES_LIMIT + sizeof TAG_FORMAT
#define ZLOG_NAME "ZeroVM"
#define ZLOG_OPTIONS (LOG_CONS | LOG_PID | LOG_NDELAY)
#define ZLOG_PRIORITY LOG_USER
#define LOWEST_VERBOSITY 1

/* develop fix for verbosity level names */
#undef LOG_DEBUG
#undef LOG_INFO
#undef LOG_WARNING

#define LOG_INSANE  (3) /* slow down logging */
#define LOG_DEBUG   (2)
#define LOG_ERROR   (1) /* mandatory message */
#define LOG_FATAL   (0) /* mandatory message */

/* todo(d'b): should be removed as soon as all logs will be revised {{ */
#define LOG_SUICIDE (3)
#define LOG_NOTE    (2)
#define LOG_INFO    (2)
#define LOG_WARNING (2)
#define NaClLog ZLogTag(__FILE__, __LINE__), ZLog
#define ErrIf LogIf
#define CHECK(bool_expr) \
  do {\
    if (!(bool_expr))\
    {\
      NaClLog(LOG_FATAL, "Fatal error in file %s, line %d: !(%s)\n",\
        __FILE__, __LINE__, #bool_expr);\
      }\
    } while (0)
/* }} */

/* initialize syslog with verbosity */
void ZLogCtor(int v);

/* close the log, reset verbosity level */
void ZLogDtor();

/* store file and line information to the log tag */
char *ZLogTag(const char *file, int line);

/* append stored debug information and log given message */
void ZLog(int priority, char *fmt, ...);

/* if condition is true, log and abort */
void FailIf(int cond, char const *fmt, ...);

/* if condition is true, log and continue */
void LogIf(int cond, char const *fmt, ...);

EXTERN_C_END

#endif /* ZLOG_H_ */
