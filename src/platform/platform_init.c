/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include "src/platform/nacl_log.h"
#include "src/platform/nacl_time.h"

void NaClPlatformInit(void) {
  NaClLogModuleInit();
  NaClTimeInit();
}

void NaClPlatformFini(void) {
  NaClTimeFini();
  NaClLogModuleFini();
}
