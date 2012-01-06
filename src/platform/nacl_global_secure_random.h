/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Secure RNG.  Global singleton.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_GLOBAL_SECURE_RANDOM_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_GLOBAL_SECURE_RANDOM_H_

#include "src/platform/nacl_sync.h"

void NaClGlobalSecureRngInit(void);

void NaClGlobalSecureRngFini(void);

uint32_t NaClGlobalSecureRngUint32(void);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_GLOBAL_SECURE_RANDOM_H_ */
