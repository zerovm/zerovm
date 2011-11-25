/*
 * Copyright 2010 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include "native_client/src/trusted/desc/pepper/nacl_pepper.h"

#include "native_client/src/shared/platform/nacl_log.h"

int64_t NaClSrpcPepperWriteRequest(uint8_t const  *file_id,
                                   int64_t        offset,
                                   int64_t        length) {
  UNREFERENCED_PARAMETER(file_id);
  UNREFERENCED_PARAMETER(offset);
  NaClLog(LOG_FATAL, "Incomplete implementation used\n");
  return length;
}

int64_t NaClSrpcPepperFtruncateRequest(uint8_t const  *file_id,
                                       int64_t        length) {
  UNREFERENCED_PARAMETER(file_id);
  NaClLog(LOG_FATAL, "Incomplete implementation used\n");
  return length;
}
