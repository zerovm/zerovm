/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_SHARED_SRPC_NACL_SRPC_INTERNAL_H_
#define NATIVE_CLIENT_SRC_SHARED_SRPC_NACL_SRPC_INTERNAL_H_

/*
 * Avoid emacs' penchant for auto-indenting extern "C" blocks.
 */
#ifndef EXTERN_C_BEGIN
#  ifdef __cplusplus
#    define EXTERN_C_BEGIN extern "C" {
#    define EXTERN_C_END   }
#  else
#    define EXTERN_C_BEGIN
#    define EXTERN_C_END
#  endif  /* __cplusplus */
#endif

EXTERN_C_BEGIN

#include <stdarg.h>
#include "native_client/src/include/portability.h"
#ifdef __native_client__
typedef int SRPC_IMC_DESC_TYPE;
#define NACL_INVALID_DESCRIPTOR (-1)
#define SRPC_DESC_MAX    IMC_USER_DESC_MAX
#else
typedef struct NaClDesc* SRPC_IMC_DESC_TYPE;
#define NACL_INVALID_DESCRIPTOR NULL
#define SRPC_DESC_MAX    NACL_ABI_IMC_USER_DESC_MAX
#endif

/* Initialize the logging facility. Returns 1 on success, 0 on failure. */
int NaClSrpcLogInit();

/* Shut down the logging facility. */
void NaClSrpcLogFini();

/*
 * NaClSrpcLog prints log messages if the detail_level is less than
 * the level set by the user in the environment variable NACL_SRPC_DEBUG.
 */
void NaClSrpcLog(int detail_level,
                 const char* fmt,
                 ...)  ATTRIBUTE_FORMAT_PRINTF(2, 3);

#define NACL_SRPC_LOG_INFO    (-1)
#define NACL_SRPC_LOG_WARNING (-2)
#define NACL_SRPC_LOG_ERROR   (-3)
#define NACL_SRPC_LOG_FATAL   (-4)


/*
 * Formats up to |buffer_size| character of |*arg| into |buffer| if
 * the current logging level is greater than |detail_level|.
 * Always writes '\0' at the end of what is written to |*buffer|.
 * If the buffer is too small to hold the contents, there may be some
 * incorrect characters at the end of the buffer.
 */
void NaClSrpcFormatArg(int detail_level,
                       const NaClSrpcArg* arg,
                       char* buffer,
                       size_t buffer_size);

/*
 * We have to do this for now, because portability.h doesn't work for
 * Native Client compilations.
 * TODO(sehr): make portability.h work for Native Client compilations.
 */
#if NACL_WINDOWS
# define UNREFERENCED_PARAMETER(P) (P)
#else
# define UNREFERENCED_PARAMETER(P) do { (void) P; } while (0)
#endif

/*
 * Maximum sendmsg buffer size.
 */
extern nacl_abi_size_t kNaClSrpcMaxImcSendmsgSize;


EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_SHARED_SRPC_NACL_SRPC_INTERNAL_H_ */
