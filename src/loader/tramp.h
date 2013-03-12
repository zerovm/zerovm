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
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */

#ifndef TRAMP_64_H__
#define TRAMP_64_H__
/*
 * text addresses, not word aligned; these are .globl symbols from the
 * assembler source, and there is no type information associated with
 * them.  we could declare these to be (void (*)(void)), i suppose,
 * but it doesn't really matter since we convert their addresses to
 * uintptr_t at every use.
 */
extern char   NaCl_trampoline_code;
extern char   NaCl_trampoline_code_end;
extern char   NaCl_trampoline_call_target;

extern char   NaClDispatchThunk;
extern char   NaClDispatchThunk_jmp_target;
extern char   NaClDispatchThunkEnd;

#endif /* TRAMP_64_H__ */
