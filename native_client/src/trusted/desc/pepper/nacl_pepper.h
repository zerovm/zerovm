/*
 * Copyright 2010 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_PEPPER_NACL_PEPPER_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_PEPPER_NACL_PEPPER_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

int64_t NaClSrpcPepperWriteRequest(uint8_t const  *file_id,
                                   int64_t        offset,
                                   int64_t        length);

int64_t NaClSrpcPepperFtruncateRequest(uint8_t const  *file_id,
                                       int64_t        length);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_DESC_PEPPER_NACL_PEPPER_H_ */
