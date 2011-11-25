/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Secure RNG abstraction.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_SECURE_RANDOM_TYPES_H__
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_SECURE_RANDOM_TYPES_H__

#include "native_client/src/include/nacl_base.h"

#include "native_client/src/shared/platform/nacl_secure_random_base.h"

/*
 * TODO(bsy): google style says use third_party, but that probably
 * won't work when we open source.  what is the right include and
 * library dependency?
 */
#ifndef USE_CRYPTO
#ifdef NACL_STANDALONE
# define USE_CRYPTO 1
#else
# define USE_CRYPTO 0
#endif
#endif

#if USE_CRYPTO
# include <openssl/aes.h>

EXTERN_C_BEGIN

/*
 * 0 < NACL_RANDOM_EXPOSE_BYTES <= AES_BLOCK_SIZE must hold.
 */
# define NACL_RANDOM_EXPOSE_BYTES  (AES_BLOCK_SIZE/2)

struct NaClSecureRng {
  struct NaClSecureRngIf  base;
  AES_KEY                 expanded_key;
  unsigned char           counter[AES_BLOCK_SIZE];
  unsigned char           randbytes[AES_BLOCK_SIZE];
  int                     nvalid;
};

EXTERN_C_END

#else

EXTERN_C_BEGIN

# define  NACL_RANDOM_BUFFER_SIZE  1024

struct NaClSecureRng {
  struct NaClSecureRngIf  base;
  unsigned char           buf[NACL_RANDOM_BUFFER_SIZE];
  int                     nvalid;
};

EXTERN_C_END

#endif
#endif
/* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_SECURE_RANDOM_TYPES_H__ */
