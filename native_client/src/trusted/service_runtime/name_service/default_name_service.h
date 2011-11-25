/*
 * Copyright 2011 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NAME_SERVICE_DEFAULT_NAME_SERVICE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NAME_SERVICE_DEFAULT_NAME_SERVICE_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"

#include "native_client/src/trusted/service_runtime/name_service/name_service.h"

int NaClDefaultNameServiceInit(struct NaClNameService *ns);

#endif
