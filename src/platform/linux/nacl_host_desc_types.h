/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.  Memory
 * mapping using descriptors.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_HOST_DESC_TYPES_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_HOST_DESC_TYPES_H_

#include "src/service_runtime/nacl_config.h"
/* d'b ###
 * a new field is added to support zmq calls
 */
struct NaClHostDesc {
  int	d;
//  char channel[NACL_CONFIG_PATH_MAX]; /* should be NULL for regular file */
};
/* d'b end */

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_LINUX_NACL_HOST_DESC_TYPES_H_ */
