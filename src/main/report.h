/*
 * finalizer, report and exit
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

#ifndef EXIT_H_
#define EXIT_H_ 1

#include "src/main/tools.h"

#define UNKNOWN_STATE "unknown error, see syslog"
#define OK_STATE "ok"

#ifdef DEBUG
#define REPORT_VALIDATOR "validator state = "
#define REPORT_RETCODE "user return code = "
#define REPORT_ETAG "etag(s) = "
#define REPORT_ACCOUNTING "accounting = "
#define REPORT_STATE "exit state = "
#define REPORT_DEBUG GetDebugString()
#else
#define REPORT_VALIDATOR ""
#define REPORT_RETCODE ""
#define REPORT_ETAG ""
#define REPORT_ACCOUNTING ""
#define REPORT_STATE ""
#define REPORT_DEBUG ""
#endif

EXTERN_C_BEGIN

/* put report to syslog instead of stdout */
void HideReport();

/* set the text for "exit state" in report */
void SetExitState(const char *state);

/* set zerovm exit code */
void SetExitCode(int code);

/* get zerovm exit code */
int GetExitCode();

/* set user session exit code */
void SetUserCode(int code);

/* set validation state (0 - passed, 1 - failed, 2 - disabled) */
void SetValidationState(int state);

/* set zerovm command line for debug purposes */
void SetDebugString(const char *s);

/* get zerovm command line */
char *GetDebugString();

/* add tag digest with given name */
void ReportTag(char *name, void *tag);

/* initialize report internals */
void ReportCtor();

/*
 * exit zerovm. if code != 0 log it and show dump. release resources
 * note: use global nap because can be invoked from signal handler
 */
void ReportDtor(int code);

EXTERN_C_END

#endif /* EXIT_H_ */
