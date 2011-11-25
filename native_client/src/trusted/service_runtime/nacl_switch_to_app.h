/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime
 */

#ifndef SERVICE_RUNTIME_NACL_SWITCH_TO_APP_H__
#define SERVICE_RUNTIME_NACL_SWITCH_TO_APP_H__ 1

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"
/* get nacl_reg_t */

EXTERN_C_BEGIN

struct NaClAppThread;

struct NaclThreadContext;

void NaClInitSwitchToApp(struct NaClApp *nap);

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
extern NORETURN void NaClSwitchAVX(struct NaClThreadContext *context);
extern NORETURN void NaClSwitchSSE(struct NaClThreadContext *context);
extern NORETURN void NaClSwitchNoSSE(struct NaClThreadContext *context);
#else
extern NORETURN void NaClSwitch(struct NaClThreadContext *context);
#endif

NORETURN void NaClStartThreadInApp(struct NaClAppThread *natp,
                                   nacl_reg_t           new_prog_ctr);

NORETURN void NaClSwitchToApp(struct NaClAppThread *natp,
                              nacl_reg_t           new_prog_ctr);

EXTERN_C_END

#endif
