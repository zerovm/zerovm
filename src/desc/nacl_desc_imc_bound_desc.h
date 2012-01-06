/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service runtime.  NaClDescImcBoundDesc subclass of NaClDesc.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_IMC_BOUND_DESC_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_IMC_BOUND_DESC_H_

#include "include/nacl_base.h"

EXTERN_C_BEGIN

struct NaClDescEffector;
struct NaClDescXferState;

/*
 * IMC bound sockets.
 */

struct NaClDescImcBoundDesc {
  struct NaClDesc           base NACL_IS_REFCOUNT_SUBCLASS;
  NaClHandle                h;
};

int NaClDescImcBoundDescInternalize(struct NaClDesc          **baseptr,
                                    struct NaClDescXferState *xfer)
    NACL_WUR;

int NaClDescImcBoundDescCtor(struct NaClDescImcBoundDesc  *self,
                             NaClHandle                   d)
    NACL_WUR;

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_IMC_BOUND_DESC_H_
