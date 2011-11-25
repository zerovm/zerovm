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

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

void NaClExit(int code);
void NaClAbort(void);

EXTERN_C_END

#endif /* NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_EXIT_H_ */
