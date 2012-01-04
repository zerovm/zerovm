/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/* @file
 *
 * Implementation of service runtime effector subclass used for all
 * application threads.
 */
#include "src/service_runtime/nacl_desc_effector_ldr.h"
#include "src/service_runtime/include/bits/mman.h"

static struct NaClDescEffectorVtbl const NaClDescEffectorLdrVtbl;  /* fwd */

int NaClDescEffectorLdrCtor(struct NaClDescEffectorLdr *self,
                            struct NaClApp             *nap) {
  self->base.vtbl = &NaClDescEffectorLdrVtbl;
  self->nap = nap;
  return 1;
}

static void NaClDescEffLdrDtor(struct NaClDescEffector *vself) {
  struct NaClDescEffectorLdr *self = (struct NaClDescEffectorLdr *) vself;

  self->nap = NULL;
  /* we did not take ownership of nap */
  vself->vtbl = (struct NaClDescEffectorVtbl const *) NULL;
}

static int NaClDescEffLdrUnmapMemory(struct NaClDescEffector  *vself,
                                     uintptr_t                sysaddr,
                                     size_t                   nbytes) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(sysaddr);
  UNREFERENCED_PARAMETER(nbytes);
  return 0;
}

static uintptr_t NaClDescEffLdrMapAnonMem(struct NaClDescEffector *vself,
                                          uintptr_t               sysaddr,
                                          size_t                  nbytes,
                                          int                     prot) {
  UNREFERENCED_PARAMETER(vself);
  return NaClHostDescMap((struct NaClHostDesc *) NULL,
                         (void *) sysaddr,
                         nbytes,
                         prot,
                         (NACL_ABI_MAP_PRIVATE |
                          NACL_ABI_MAP_ANONYMOUS |
                          NACL_ABI_MAP_FIXED),
                         (off_t) 0);
}

static struct NaClDescEffectorVtbl const NaClDescEffectorLdrVtbl = {
  NaClDescEffLdrDtor,
  NaClDescEffLdrUnmapMemory,
  NaClDescEffLdrMapAnonMem,
};
