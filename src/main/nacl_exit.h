/*
 * Copyright 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */
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

/*
 * This module implementes a platform specific exit and abort, to terminate
 * the process as quickly as possible when an exception is detected.
 */
#ifndef NACL_EXIT_H_
#define NACL_EXIT_H_ 1

#include "src/main/nacl_base.h"

#define UNKNOWN_STATE "unknown error, see syslog"
#define OK_STATE "ok"

EXTERN_C_BEGIN

/* exit zerovm. if code != 0 log it */
void NaClExit(int code);

/* set the text for "exit state" in report */
void SetExitState(const char *state);

/* get the "exit state" message */
const char *GetExitState();

/* set zerovm exit code */
void SetExitCode(int code);

/* get zerovm exit code */
int GetExitCode();

EXTERN_C_END

#endif /* NACL_EXIT_H_ */
