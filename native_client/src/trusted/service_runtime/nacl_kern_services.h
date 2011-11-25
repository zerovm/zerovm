/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * A simple service for "kernel services".  The socket address will be
 * available to the NaCl module.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_KERN_SERVICES_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_KERN_SERVICES_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/simple_service/nacl_simple_service.h"

EXTERN_C_BEGIN

struct NaClKernService {
  struct NaClSimpleService  base NACL_IS_REFCOUNT_SUBCLASS;

  struct NaClApp            *nap;
};

int NaClKernServiceCtor(
    struct NaClKernService      *self,
    NaClThreadIfFactoryFunction thread_factory_fn,
    void                        *thread_factory_data,
    struct NaClApp              *nap);

void NaClKernServiceDtor(struct NaClRefCount *vself);

extern struct NaClSimpleServiceVtbl const kNaClKernServiceVtbl;

EXTERN_C_END

#endif
