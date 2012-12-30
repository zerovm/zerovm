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

#ifndef SERVICE_RUNTIME_NACL_SYSCALL_HANDLERS_H__
#define SERVICE_RUNTIME_NACL_SYSCALL_HANDLERS_H__ 1

struct NaClSyscallTableEntry {
  int32_t (*handler)(struct NaClApp *nap);
};

/* these are defined in the platform specific code */
extern struct NaClSyscallTableEntry nacl_syscall[];

void NaClSyscallTableInit();

#endif
