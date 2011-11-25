/*
 * Copyright 2009 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_TEXT_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_TEXT_H_

#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/nacl_error_code.h"

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

int32_t NaClTextDyncodeCreate(struct NaClApp *nap,
                              uint32_t       dest,
                              void           *code_copy,
                              uint32_t       size);

int32_t NaClTextSysDyncode_Create(struct NaClAppThread *natp,
                                  uint32_t             dest,
                                  uint32_t             src,
                                  uint32_t             size);

int32_t NaClTextSysDyncode_Modify(struct NaClAppThread *natp,
                                  uint32_t             dest,
                                  uint32_t             src,
                                  uint32_t             size);

int32_t NaClTextSysDyncode_Delete(struct NaClAppThread *natp,
                                  uint32_t             dest,
                                  uint32_t             size);

EXTERN_C_END

#endif
