/*
 * Copyright 2011 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NAME_SERVICE_DEFAULT_NAME_SERVICE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NAME_SERVICE_DEFAULT_NAME_SERVICE_H_

#include "include/nacl_base.h"
#include "include/portability.h"

#include "trusted/desc/nacl_desc_base.h"

#include "trusted/service_runtime/name_service/name_service.h"

int NaClDefaultNameServiceInit(struct NaClNameService *ns);

#endif
