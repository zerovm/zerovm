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

/*
 * Yields an lvalue of the vtable pointer, when given a subclass of
 * NaClRefCount and a pointer to an instance -- with the convention
 * that the vtable pointer is at the first word.
 *
 * Since we have the naming convention that struct Type contains an
 * element of type struct TypeVtbl *, we make it so that less typing
 * is required for the use of the macro and automatically constructing
 * the vtable type name from the subclass name.
 */
#define NACL_VTBL(type, ptr) \
  (*(struct type ## Vtbl const **) (void *) ptr)

struct NaClRefCountVtbl;

struct NaClRefCount {
  struct NaClRefCountVtbl const *vtbl NACL_IS_REFCOUNT_SUBCLASS;
  /* private */
  size_t                        ref_count;
};

struct NaClRefCountVtbl {
  void (*Dtor)(struct NaClRefCount  *vself);
};

///*
// * Placement new style ctor; creates w/ ref_count of 1.
// *
// * The subclasses' ctor must call this base class ctor during their
// * contruction.
// */
//int NaClRefCountCtor(struct NaClRefCount *nrcp) NACL_WUR;

struct NaClRefCount *NaClRefCountRef(struct NaClRefCount *nrcp);

/* when ref_count reaches zero, will call dtor and free */
void NaClRefCountUnref(struct NaClRefCount *nrcp);

extern struct NaClRefCountVtbl const kNaClRefCountVtbl;

EXTERN_C_END

#endif
