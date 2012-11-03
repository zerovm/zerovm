/*
 * Copyright 2008  The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Service Runtime API.  Time types.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_TIME_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_SYS_TIME_H_

#include <stdint.h>

typedef int32_t nacl_abi_suseconds_t;
typedef int64_t nacl_abi___time_t;
typedef nacl_abi___time_t nacl_abi_time_t;

struct nacl_abi_timeval {
  nacl_abi_time_t      nacl_abi_tv_sec;
  nacl_abi_suseconds_t nacl_abi_tv_usec;
};

#endif
