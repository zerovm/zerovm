/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_NACL_BASE_NACL_REFCOUNT_H_
#define NATIVE_CLIENT_SRC_TRUSTED_NACL_BASE_NACL_REFCOUNT_H_

#include "include/nacl_base.h"
#include "include/portability.h"

#define NACL_IS_REFCOUNT_SUBCLASS

EXTERN_C_BEGIN

struct NaClRefCountVtbl;

struct NaClRefCount {
  struct NaClRefCountVtbl const *vtbl NACL_IS_REFCOUNT_SUBCLASS;
  /* private */
  size_t                        ref_count;
};

struct NaClRefCountVtbl {
  void (*Dtor)(struct NaClRefCount  *vself);
};

struct NaClRefCount *NaClRefCountRef(struct NaClRefCount *nrcp);

/* when ref_count reaches zero, will call dtor and free */
void NaClRefCountUnref(struct NaClRefCount *nrcp);

EXTERN_C_END

#endif
