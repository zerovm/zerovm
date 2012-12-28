/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Server Runtime globals.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_GLOBALS_H__
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_GLOBALS_H__

#include <stdint.h>
#include <setjmp.h>

EXTERN_C_BEGIN

struct NaClThreadContext;
struct NaClAppThread;
struct NaClApp;

/* d'b: added global variables */
struct NaClThreadContext    *nacl_user; /* user registers storage */
struct NaClThreadContext    *nacl_sys; /* zerovm registers storage */
struct NaClApp              *gnap; /* pointer to global NaClApp object */
int64_t                     syscallback; /* 0 if not installed */
jmp_buf                     user_exit; /* part of trap() exit */
/* variables above will be initialized later */

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_GLOBALS_H__ */
