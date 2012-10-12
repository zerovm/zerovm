/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_BASE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_BASE_H_

#include "src/include/nacl_base.h"
#include "src/platform/nacl_host_desc.h" /* for nacl_off64_t */
#include "src/nacl_base/nacl_refcount.h"

EXTERN_C_BEGIN

struct NaClDesc {
  struct NaClRefCount base NACL_IS_REFCOUNT_SUBCLASS;
};

struct NaClDesc *NaClDescRef(struct NaClDesc *ndp);

/* when ref_count reaches zero, will call dtor and free */
void NaClDescUnref(struct NaClDesc *ndp);

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_BASE_H_
