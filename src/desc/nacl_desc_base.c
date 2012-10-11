/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.  Memory
 * mapping using descriptors.
 */

#include "src/desc/nacl_desc_base.h"
#include "src/platform/nacl_log.h"
#include "src/nacl_base/nacl_refcount.h"
#include "src/service_runtime/include/sys/errno.h"

struct NaClDesc *NaClDescRef(struct NaClDesc *ndp) {
  return (struct NaClDesc *) NaClRefCountRef(&ndp->base);
}

void NaClDescUnref(struct NaClDesc *ndp) {
  NaClRefCountUnref(&ndp->base);
}
