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
#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_MEMORY_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_MEMORY_H_ 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
* We do not use posix_memalign but instead directly attempt to mmap
* (or VirtualAlloc) memory into aligned addresses, since we want to be
* able to munmap pages to map in shared memory pages for the NaCl
* versions of shmat or mmap, esp if SHM_REMAP is used.  Note that the
* Windows ABI has 4KB pages for operations like page protection, but
* 64KB allocation granularity (see nacl_config.h), and since we want
* host-OS indistinguishability, this means we inherit this restriction
* into our least-common-denominator design.
*/
#define MAX_RETRIES     1024

int NaCl_page_alloc_intern_flags(void   **p,
                                 size_t size,
                                 int    map_flags);
void  NaCl_page_free(void     *p,
                     size_t   num_bytes);

int   NaCl_mprotect(void          *addr,
                    size_t        len,
                    int           prot) NACL_WUR;

int   NaCl_madvise(void           *start,
                   size_t         length,
                   int            advice) NACL_WUR;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*  NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_MEMORY_H_ */
