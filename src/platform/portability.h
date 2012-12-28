/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file should be at the top of the #include group, followed by
 * standard system #include files, then by native client specific
 * includes.
 *
 * TODO(gregoryd): explain why.  (Something to do with windows include
 * files, to be reconstructed.)
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_H_
#define NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_H_ 1

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "src/main/nacl_base.h"
#include "src/platform/nacl_compiler_annotations.h"

#ifndef SIZE_T_MAX
# define SIZE_T_MAX ((size_t) -1)
#endif

#include <inttypes.h>

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_H_ */
