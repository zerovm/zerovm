/*
 * Copyright 2009 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Secure Runtime
 */

#ifndef __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_SEL_RT_H__
#define __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_SEL_RT_H__ 1

#include "native_client/src/include/portability.h"

#if NACL_BUILD_SUBARCH == 32
# include "native_client/src/trusted/service_runtime/arch/x86_32/sel_rt_32.h"
#elif NACL_BUILD_SUBARCH == 64
# include "native_client/src/trusted/service_runtime/arch/x86_64/sel_rt_64.h"
#else
# error "Woe to the service runtime.  Is it running on a 128-bit machine?!?"
#endif

nacl_reg_t NaClGetStackPtr(void);

#endif /* __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_SEL_RT_H__ */
