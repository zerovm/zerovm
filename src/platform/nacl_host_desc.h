/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.  Memory
 * mapping using descriptors.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_HOST_DESC_H__
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_HOST_DESC_H__
#include <stdint.h>
#include "src/platform/linux/nacl_host_desc_types.h"

/*
 * see NACL_MAP_PAGESIZE from nacl_config.h; map operations must be aligned
 */

EXTERN_C_BEGIN

/*
 * off64_t in linux, off_t in osx and __int64_t in win
 */
typedef int64_t nacl_off64_t;

/*
 * We do not explicitly provide an abstracted version of a
 * host-independent stat64 structure.  Instead, it is up to the user
 * of the nacl_host_desc code to not use anything but the
 * POSIX-blessed fields, to know that the shape/size may differ across
 * platforms, and to know that the st_size field is a 64-bit value
 * compatible w/ nacl_off64_t above.
 */
typedef struct stat64 nacl_host_stat_t;

extern int NaClXlateErrno(int errnum);

#ifndef __native_client__ /* these functions are not exposed to NaCl modules
                           * (see TODO comment above)
                           */

/*
 * Maps NACI_ABI_ versions of the mmap prot argument to host ABI versions
 * of the bit values
 */

extern int NaClProtMap(int abi_prot);

EXTERN_C_END

#endif  /* defined __native_client__ */

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_HOST_DESC_H__ */
