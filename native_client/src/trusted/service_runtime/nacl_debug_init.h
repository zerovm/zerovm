/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_DEBUG_INIT_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_DEBUG_INIT_H_ 1

#include "native_client/src/trusted/service_runtime/sel_ldr.h"

EXTERN_C_BEGIN

/*
 * Enables the debug stub.  If this is called, we do not guarantee
 * security to the same extent that we normally would.
 */
int NaClDebugInit(struct NaClApp *nap,
                  int argc, char const *const argv[],
                  int envc, char const *const envv[]);

EXTERN_C_END

#endif
