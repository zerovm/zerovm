/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/nacl_base/nacl_refcount.h"
#include "src/platform/nacl_log.h"

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
