/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include "shared/platform/nacl_log.h"
#include "shared/platform/nacl_time.h"
#include "shared/platform/nacl_secure_random.h"
#include "shared/platform/nacl_global_secure_random.h"

void NaClPlatformInit(void) {
  NaClLogModuleInit();
  NaClTimeInit();
  NaClSecureRngModuleInit();
  NaClGlobalSecureRngInit();
}

void NaClPlatformFini(void) {
  NaClGlobalSecureRngFini();
  NaClSecureRngModuleFini();
  NaClTimeFini();
  NaClLogModuleFini();
}
