/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * A NaClDesc subclass that exposes the platform secure RNG
 * implementation.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_RNG_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_RNG_H_

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/shared/platform/nacl_secure_random.h"

/*
 * NaClSecureRngModuleInit must have been called, perhaps indirectly
 * through NaClPlatformInit.
 */

EXTERN_C_BEGIN

/*
 * This is not a cryptographically forward secure rng, but it should
 * be cryptographically secure.
 */
struct NaClDescRng {
  struct NaClDesc       base NACL_IS_REFCOUNT_SUBCLASS;
  struct NaClSecureRng  rng;
};

int NaClDescRngCtor(struct NaClDescRng  *self);

int NaClDescRngInternalize(struct NaClDesc          **out_desc,
                           struct NaClDescXferState *xfer);

EXTERN_C_END

#endif
