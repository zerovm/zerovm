/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "src/platform_qualify/linux/sysv_shm_and_mmap.h"

/*
 * Returns 1 if the operating system can run Native Client modules.
 */
int NaClOsIsSupported()
{
  return !NaClPlatformQualifySysVShmAndMmapHasProblems();
}
