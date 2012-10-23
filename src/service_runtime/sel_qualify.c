/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <errno.h>
#include "src/service_runtime/sel_qualify.h"
#include "src/service_runtime/zlog.h"
#include "src/platform_qualify/nacl_dep_qualify.h"
#include "src/platform_qualify/nacl_os_qualify.h"

void NaClRunSelQualificationTests()
{
  /* fail if Operating system platform is not supported */
  ZLOGFAIL(!NaClOsIsSupported(), EFAULT, "os not supported");

  /* fail if Data Execution Prevention is required but is not supported */
  ZLOGFAIL(!NaClCheckDEP(), EFAULT, "dep not supported");
}
