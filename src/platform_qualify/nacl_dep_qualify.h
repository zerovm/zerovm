/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */


#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_DEP_QUALIFY_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_DEP_QUALIFY_H_

/*
 * Two of our target architectures (x86-64 and ARM) require that data not be
 * executable for our sandbox to hold.  Every vendor calls this something
 * different.  For the purposes of platform qualification, we use the term
 * 'data execution prevention,' or DEP.
 *
 * This file presents the common interface for DEP-check routines.  The
 * implementations differ greatly across platforms and architectures.  See the
 * notes on each function for details on where implementations live.
 */

#include <stddef.h>
#include "src/include/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Checks that Data Execution Prevention is working as required by the
 * architecture.  On some architectures this is a no-op.
 */
int NaClCheckDEP();

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_NACL_DEP_QUALIFY_H_ */
