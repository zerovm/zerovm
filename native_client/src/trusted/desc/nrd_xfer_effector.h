/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NRD xfer effector for trusted code use.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NRD_XFER_EFFECTOR_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NRD_XFER_EFFECTOR_H_

#include "native_client/src/include/nacl_base.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_effector.h"

EXTERN_C_BEGIN

struct NaClNrdXferEffector {
  struct NaClDescEffector base;
};

/* bool: returns non-zero for success, ups refcount on src_desc */
int NaClNrdXferEffectorCtor(struct NaClNrdXferEffector  *self);

EXTERN_C_END

#endif /* NATIVE_CLIENT_SRC_TRUSTED_DESC_NRD_XFER_EFFECTOR_H_ */
