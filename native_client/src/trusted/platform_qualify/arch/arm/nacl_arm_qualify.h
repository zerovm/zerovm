/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_ARCH_ARM_NACL_ARM_QUALIFY_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_ARCH_ARM_NACL_ARM_QUALIFY_H_

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Returns 1 if the __aeabi_read_tp ABI call safely avoids using the stack.
 */
int NaClQualifyReadTp(void);

EXTERN_C_END

#endif
