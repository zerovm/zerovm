/*
 * Copyright 2009 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_TEXT_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_TEXT_H_

#include "src/service_runtime/nacl_error_code.h"

EXTERN_C_BEGIN

struct NaClApp;
struct NaClAppThread;

struct NaClDynamicRegion {
  uintptr_t start;
  size_t size;
  int delete_generation;
};

/*
 * Create a shared memory descriptor and map it into the text region
 * of the address space.  This implies that the text size must be a
 * multiple of NACL_MAP_PAGESIZE.
 */
NaClErrorCode NaClMakeDynamicTextShared(struct NaClApp *nap);

struct NaClDescEffectorShm;
int NaClDescEffectorShmCtor(struct NaClDescEffectorShm *self);

EXTERN_C_END

#endif
