/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/nacl_base/nacl_refcount.h"
#include "src/platform/nacl_log.h"

int NaClRefCountCtor(struct NaClRefCount *self) {
  NaClLog(4, "NaClRefCountCtor(0x%08"NACL_PRIxPTR").\n", (uintptr_t) self);
  self->ref_count = 1;
  self->vtbl = (struct NaClRefCountVtbl *) NULL;
  return 0;
}

static void NaClRefCountDtor(struct NaClRefCount  *self) {
  NaClLog(4, "NaClRefCountDtor(0x%08"NACL_PRIxPTR"), refcount %"NACL_PRIdS
          ", destroying.\n",
          (uintptr_t) self,
          self->ref_count);
  /*
   * NB: refcount could be non-zero.  Here's why: if a subclass's Ctor
   * fails, it will have already run NaClRefCountCtor and have
   * initialized the mutex and set the ref_count to 1.  Because Unref
   * would free memory and Ctors aren't factories, the subclass Ctor
   * cannot just invoke NaClRefCountUnref; instead, it must directly
   * invoke the base class Dtor.
   *
   * We do, however, expect the ref_count to be either 0 or 1.
   */
  switch (self->ref_count) {
    case 0:
      break;
    case 1:
      NaClLog(LOG_WARNING,
              ("NaClRefCountDtor invoked on a generic refcounted"
               " object at 0x%08"NACL_PRIxPTR" with refcount 1."
               "  This legitimately occurs only during subclass"
               " ctor failures.\n"),
              (uintptr_t) self);
      break;
    default:
      NaClLog(LOG_FATAL,
              ("NaClRefCountDtor invoked on a generic refcounted"
               " object at 0x%08"NACL_PRIxPTR" with non-zero"
               " reference count (%"NACL_PRIdS")\n"),
              (uintptr_t) self,
              self->ref_count);
  }

  self->vtbl = (struct NaClRefCountVtbl const *) NULL;
}

struct NaClRefCountVtbl const kNaClRefCountVtbl = {
  NaClRefCountDtor,
};

struct NaClRefCount *NaClRefCountRef(struct NaClRefCount *nrcp) {
  NaClLog(4, "NaClRefCountRef(0x%08"NACL_PRIxPTR").\n",
          (uintptr_t) nrcp);
  if (0 == ++nrcp->ref_count) {
    NaClLog(LOG_FATAL, "NaClRefCountRef integer overflow\n");
  }
  return nrcp;
}

void NaClRefCountUnref(struct NaClRefCount *nrcp) {
  int destroy;

  NaClLog(4, "NaClRefCountUnref(0x%08"NACL_PRIxPTR").\n",
          (uintptr_t) nrcp);
  if (0 == nrcp->ref_count) {
    NaClLog(LOG_FATAL,
            ("NaClRefCountUnref on 0x%08"NACL_PRIxPTR
             ", refcount already zero!\n"),
            (uintptr_t) nrcp);
  }
  destroy = (0 == --nrcp->ref_count);
  if (destroy) {
    (*nrcp->vtbl->Dtor)(nrcp);
    free(nrcp);
  }
}
