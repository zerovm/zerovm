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
#include "src/loader/sel_ldr.h"

#define UNKNOWN_STATE "unknown error, see syslog"
#define OK_STATE "ok"

EXTERN_C_BEGIN

/* assign file handle to put report */
void SetReportHandle(int handle);

/* put report to syslog instead of stdout */
void ReportMode(int mode);

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
void SetCmdString(GString *s);

/* add tag digest with given name */
void ReportTag(char *name, void *tag);

/* initialize report internals */
void ReportCtor();

/*
 * report intermediate session statistics as often as defined by QUANT
 * TODO(d'b): self-initializing. move it to ReportCtor()
 * note: will not drop reports if waiting time is more than QUANT,
 *       however reports number can be lesser than run time in seconds
 */
void FastReport();

/* full report (declared for daemon) */
void Report(struct NaClApp *nap);

/*
 * exit zerovm. if code != 0 log it and show dump. release resources
 * note: use global nap because can be invoked from signal handler
 */
void ReportDtor(int code);

EXTERN_C_END

#endif /* EXIT_H_ */
