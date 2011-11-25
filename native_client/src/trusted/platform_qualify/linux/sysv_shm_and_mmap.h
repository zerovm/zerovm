/*
 * Copyright 2009 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_LINUX_SYSV_SHM_AND_MMAP_H__
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_LINUX_SYSV_SHM_AND_MMAP_H__

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * NaClPlatformQualifySysVShmAndMmapHasProblems returns 0 if there are
 * no problems, and an integer (as well as printing to standard error)
 * representing the error that occurred.  We do not provide any
 * user-viewable / -interpretable string for this error code.
 */
int NaClPlatformQualifySysVShmAndMmapHasProblems(void);

EXTERN_C_END

#endif
