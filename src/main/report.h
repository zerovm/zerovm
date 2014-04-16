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

struct ReportSetup {
  uint8_t zvm_code;
  uint8_t validation_state;
  uint8_t daemon_state;
  uint8_t mode;
  int handle;
  uint64_t user_code;
  char *zvm_state;
  char *cmd;
};

/* returns report setup structure. TODO(d'b): better solution? */
struct ReportSetup *ReportSetupPtr();

/* add tag digest with given name */
void ReportTag(char *name, void *tag);

/* initialize report internals */
void ReportCtor();

/* full report (declared for daemon) */
void Report(struct NaClApp *nap);

/*
 * exit zerovm. if code != 0 log it and show dump. release resources
 * note: use global nap because can be invoked from signal handler
 */
void ReportDtor(int code);

EXTERN_C_END

#endif /* EXIT_H_ */
