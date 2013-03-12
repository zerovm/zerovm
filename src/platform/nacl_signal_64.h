/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
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
 * NaCl Signal Context
 */

#ifndef NACL_SIGNAL_64_H__
#define NACL_SIGNAL_64_H__ 1

#include "src/platform/portability.h"

/*
 * Architecture specific context object.  Register order matches that
 * found in src/src/gdb_rsp/abi.cc, which allows us to use an
 * abi context (GDB ordered context), and a signal context interchangably.
 * In addition, we use common names for the stack and program counter to
 * allow functions which use them to avoid conditional compilation.
 * todo(d'b): it almost the duplicate of struct NaClThreadContext. there can be only one.
 */
struct NaClSignalContext {
  uint64_t rax;
  uint64_t rbx;
  uint64_t rcx;
  uint64_t rdx;
  uint64_t rsi;
  uint64_t rdi;
  uint64_t rbp;
  uint64_t stack_ptr;
  uint64_t r8;
  uint64_t r9;
  uint64_t r10;
  uint64_t r11;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;
  uint64_t prog_ctr;
  uint32_t flags;
  uint32_t cs;
  uint32_t ss;
  uint32_t ds;
  uint32_t es;
  uint32_t fs;
  uint32_t gs;
};

#endif /* NACL_SIGNAL_64_H__ */
