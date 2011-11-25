/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_STACK_SAFETY_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_STACK_SAFETY_H_

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * NB: Relying code should open-code -- not use a function call -- in
 * assembly the necessary code to read nacl_thread_on_safe_stack.
 */

void NaClStackSafetyInit(void);
void NaClStackSafetyFini(void);
void NaClStackSafetyNowOnUntrustedStack(void);
void NaClStackSafetyNowOnTrustedStack(void);

EXTERN_C_END

#endif
