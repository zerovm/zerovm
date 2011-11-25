/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NRD xfer effector for trusted code use.
 */

#include "native_client/src/include/nacl_base.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_effector.h"
#include "native_client/src/trusted/desc/nrd_xfer_effector.h"

#include "native_client/src/shared/platform/nacl_log.h"

/* fwd */
static struct NaClDescEffectorVtbl NaClNrdXferEffectorVtbl;

int NaClNrdXferEffectorCtor(struct NaClNrdXferEffector  *self) {
  self->base.vtbl = &NaClNrdXferEffectorVtbl;
  return 1;
}

static void NaClNrdXferEffectorDtor(struct NaClDescEffector *vself) {
  UNREFERENCED_PARAMETER(vself);
}

static int NaClNrdXferEffectorUnmapMemory(struct NaClDescEffector *vself,
                                          uintptr_t               sysaddr,
                                          size_t                  nbytes) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(sysaddr);
  UNREFERENCED_PARAMETER(nbytes);
  return 0;
}

static uintptr_t NaClNrdXferEffectorMapAnonymousMemory(
    struct NaClDescEffector *vself,
    uintptr_t               sysaddr,
    size_t                  nbytes,
    int                     prot) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(sysaddr);
  UNREFERENCED_PARAMETER(nbytes);
  UNREFERENCED_PARAMETER(prot);
  return 0;
}

static struct NaClDescEffectorVtbl NaClNrdXferEffectorVtbl = {
  NaClNrdXferEffectorDtor,
  NaClNrdXferEffectorUnmapMemory,
  NaClNrdXferEffectorMapAnonymousMemory,
};
