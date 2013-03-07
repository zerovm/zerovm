/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */
/*
 * Copyright (c) 2012, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
struct NaClApp;

/* d'b: added global variables */
struct NaClThreadContext    *nacl_user; /* user registers storage */
struct NaClThreadContext    *nacl_sys; /* zerovm registers storage */
struct NaClApp              *gnap; /* pointer to global NaClApp object */
jmp_buf                     user_exit; /* part of trap() exit */
/* variables above will be initialized later */

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_GLOBALS_H__ */
