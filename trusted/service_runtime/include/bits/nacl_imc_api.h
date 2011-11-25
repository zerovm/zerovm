/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  IMC API.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_BITS_NACL_IMC_API_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_BITS_NACL_IMC_API_H_

/*
 * This file contains the values that the service runtime ABI uses.
 * It may be included from either an assembly or C source file, so
 * only #define statements are usable here.
 */

/* these values must match src/shared/imc/nacl_imc{,_c}.h */
#define NACL_ABI_RECVMSG_DATA_TRUNCATED 0x1
#define NACL_ABI_RECVMSG_DESC_TRUNCATED 0x2

#define NACL_ABI_IMC_NONBLOCK           0x1

#endif
