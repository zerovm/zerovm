/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "src/platform/nacl_sync.h"
#include "src/service_runtime/include/sys/time.h"

/* Condition variable C API */

int NaClCondVarCtor(struct NaClCondVar  *cvp) {
  if (0 != pthread_cond_init(&cvp->cv, (pthread_condattr_t *) 0)) {
    return 0;
  }
#if NACL_COND_BROADCAST_HACK
  cvp->waiting = 0;
#endif
  return 1;
}

void NaClCondVarDtor(struct NaClCondVar *cvp) {
  pthread_cond_destroy(&cvp->cv);
}
