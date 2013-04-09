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

/* NaCl Simple/secure ELF loader (NaCl SEL) */

#ifndef __SEL_ADDRSPACE_H__
#define __SEL_ADDRSPACE_H__ 1

struct NaClApp; /* fwd */

/* deallocate all user memory (84gb) */
void NaClFreeAddrSpace(struct NaClApp *nap);

/* allocate user memory chunk (84gb) */
void NaClAllocAddrSpace(struct NaClApp *nap);

/* Apply memory protection to memory regions */
void NaClMemoryProtection(struct NaClApp *nap);

#endif
