/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/service_runtime/sel_qualify.h"
#include "src/platform_qualify/nacl_dep_qualify.h"
#include "src/platform_qualify/nacl_os_qualify.h"

NaClErrorCode NaClRunSelQualificationTests() {
  if (!NaClOsIsSupported()) {
    return LOAD_UNSUPPORTED_OS_PLATFORM;
  }

  if (!NaClCheckDEP()) {
    return LOAD_DEP_UNSUPPORTED;
  }

  return LOAD_OK;
}
