/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */


#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_OS_QUALIFY_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_OS_QUALIFY_H_

#include "include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Returns 1 if the operating system can run Native Client modules.
 */
int NaClOsIsSupported();

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_OS_QUALIFY_H_ */
