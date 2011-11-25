/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/sel_qualify.h"

#include "native_client/src/trusted/platform_qualify/nacl_dep_qualify.h"
#include "native_client/src/trusted/platform_qualify/nacl_os_qualify.h"
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
#include "native_client/src/trusted/platform_qualify/arch/arm/nacl_arm_qualify.h"
#endif


NaClErrorCode NaClRunSelQualificationTests() {
  if (!NaClOsIsSupported()) {
    return LOAD_UNSUPPORTED_OS_PLATFORM;
  }

  if (!NaClCheckDEP()) {
    return LOAD_DEP_UNSUPPORTED;
  }

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  if (!NaClQualifyReadTp()) {
    return LOAD_UNSUPPORTED_OS_PLATFORM;
  }
#endif

  return LOAD_OK;
}
