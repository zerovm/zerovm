/*
 * Copyright 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * This module implementes a platform specific exit and abort, to terminate
 * the process as quickly as possible when an exception is detected.
 */
#ifndef NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_EXIT_H_
#define NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_EXIT_H_ 1

#include "src/include/nacl_base.h"

#define UNKNOWN_STATE "unknown error, see syslog"
#define OK_STATE "ok"

EXTERN_C_BEGIN

/* exit zerovm. if code != 0 log it */
void NaClExit(int code);

/* set the text for "exit state" in report */
void SetExitState(const char *state);

/* get the "exit state" message */
const char *GetExitState();

EXTERN_C_END

#endif /* NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_EXIT_H_ */
