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
 * NaCl Simple/secure ELF loader (NaCl SEL) memory protection abstractions.
 */
#ifndef SEL_MEMORY_H_
#define SEL_MEMORY_H_ 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* d'b: a global function now */
int NaCl_page_alloc_intern_flags(void **p, size_t size, int map_flags);

int NaCl_page_free(void *p, size_t num_bytes);

/*
 * This is critical to make the text region non-writable, and the data
 * region read/write but no exec.  Of course, some kernels do not
 * respect the lack of PROT_EXEC.
 */
int NaCl_mprotect(void *addr, size_t len, int prot) NACL_WUR;

int NaCl_madvise(void *start, size_t length, int advice) NACL_WUR;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*  SEL_MEMORY_H_ */
