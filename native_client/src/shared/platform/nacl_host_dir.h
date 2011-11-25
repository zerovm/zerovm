/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Host Directory descriptor / Handle abstraction.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_HOST_DIR_H__
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_HOST_DIR_H__

#include <errno.h>

#include "native_client/src/include/portability.h"

#include "native_client/src/include/nacl_base.h"

#if NACL_LINUX
# include "native_client/src/shared/platform/linux/nacl_host_dir_types.h"
#elif NACL_OSX
# include "native_client/src/shared/platform/osx/nacl_host_dir_types.h"
#elif NACL_WINDOWS
# include "native_client/src/shared/platform/win/nacl_host_dir_types.h"
#endif

EXTERN_C_BEGIN

struct nacl_abi_stat;

/* TODO: this is also declared in nacl_host_desc.h */
extern int NaClXlateErrno(int errnum);

/*
 * Constructor for a NaClHostDir object.
 *
 * path should be a host-OS pathname to a directory.  No validation is
 * done.
 *
 * Uses raw syscall return convention, so returns 0 for success and
 * non-zero (usually -NACL_ABI_EINVAL) for failure.
 *
 * We cannot return the platform descriptor/handle and be consistent
 * with a largely POSIX-ish interface, since e.g. windows handles may
 * be negative and might look like negative errno returns.
 *
 * Underlying host-OS functions: opendir(Mac) / open(Linux) / FindFirstFile
 */
extern int NaClHostDirOpen(struct NaClHostDir *d,
                           char               *path);

/*
 * Read data from an opened directory into a memory buffer.
 *
 * Underlying host-OS functions: readdir(Mac) / getdents(Linux) / FindNextFile
 */
extern ssize_t NaClHostDirGetdents(struct NaClHostDir *d,
                                   void               *buf,
                                   size_t             len);

/*
 * Dtor for the NaClHostDir object. Close the directory.
 *
 * Underlying host-OS functions:  closedir(Mac) / close(Linux) / FindClose
 */
extern int NaClHostDirClose(struct NaClHostDir  *d);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_HOST_DIR_H__ */
