/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
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
 * NaCl Service Runtime
 */

#ifndef SWITCH_TO_APP_H__
#define SWITCH_TO_APP_H__ 1

#include "src/loader/sel_ldr.h"
#include "src/loader/sel_rt.h"
/* get nacl_reg_t */

EXTERN_C_BEGIN

void InitSwitchToApp(struct NaClApp *nap);

extern NORETURN void SwitchAVX(struct ThreadContext *context);
extern NORETURN void SwitchSSE(struct ThreadContext *context);

NORETURN void SwitchToApp(struct NaClApp *nap, nacl_reg_t new_prog_ctr);

EXTERN_C_END

#endif /* SWITCH_TO_APP_H__ */
