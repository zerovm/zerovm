/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL) misc utilities.  Inlined
 * functions.  Needed since C++ static inline declaractions can
 * generate undefined external references (at least as of gcc 4.3.3
 * used in Jaunty), in the debug build.
 */

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_base.h"

#include <sys/types.h>

#include "native_client/src/trusted/service_runtime/nacl_config.h"

#define static
#undef INLINE
#define INLINE

#include "native_client/src/trusted/service_runtime/sel_util-inl.h"
