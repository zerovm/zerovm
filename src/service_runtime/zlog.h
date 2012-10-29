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

/*
 * ZLOG - add file and line info to given message and put it to syslog
 * ZLOGS - put given message to syslog
 * ZLOGIF - check condition and, if true, ZLOG it
 * ZLOGFAIL - check condition, if true, ZLOG it and abort
 */
#define ZLOG ZLogTag(__FILE__, __LINE__), ZLog
#define ZLOGIF ZLogTag(__FILE__, __LINE__), LogIf
#define ZLOGFAIL ZLogTag(__FILE__, __LINE__), FailIf
#define ZLOGS ZLogTag(NULL, 0), ZLog
#define ZENTER ZLOG(LOG_INSANE, "entered")
#define ZLEAVE ZLOG(LOG_INSANE, "left")

#define FAILED_MSG "check failed"
#define ZLOG_NAME "ZeroVM"
#define ZLOG_OPTIONS (LOG_CONS | LOG_PID | LOG_NDELAY)
#define ZLOG_FACILITY LOG_USER
#define ZLOG_PRIORITY LOG_ERROR
#define TAG_FORMAT "%s %d: "
#define LOG_MSG_LIMIT 0x1000

/* develop fix for verbosity level names */
#undef LOG_DEBUG

#define LOG_INSANE  (3) /* slows down logging */
#define LOG_DEBUG   (2)
#define LOG_ERROR   (1) /* mandatory message */
#define LOG_FATAL   (0) /* mandatory message */

/* initialize syslog with verbosity */
void ZLogCtor(int v);

/* close the log, reset verbosity level */
void ZLogDtor();

/*
 * store file and line information to the log tag
 * should be used from ZLOG, ZLOGS, ZLOGIF, ZLOGFAIL
 */
void ZLogTag(const char *file, int line);

/*
 * append stored debug information and log given message
 * should be used from ZLOG, ZLOGS
 */
void ZLog(int priority, char *fmt, ...);

/* if condition is true, log and abort. should be used from ZLOGFAIL */
void FailIf(int cond, int err, char const *fmt, ...);

/* if condition is true, log and continue. should be used from ZLOGIF */
void LogIf(int cond, char const *fmt, ...);

EXTERN_C_END

#endif /* ZLOG_H_ */
