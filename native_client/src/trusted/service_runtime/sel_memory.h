/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL) memory protection abstractions.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_MEMORY_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_MEMORY_H_ 1

#include "native_client/src/include/nacl_compiler_annotations.h"

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

int   NaCl_find_prereserved_sandbox_memory(void   **p,
                                           size_t num_bytes);

int   NaCl_page_alloc(void    **p,
                      size_t  num_bytes) NACL_WUR;

int   NaCl_page_alloc_randomized(void   **p,
                                 size_t num_bytes) NACL_WUR;

int   NaCl_page_alloc_at_addr(void **p,
                              size_t  size) NACL_WUR;

void  NaCl_page_free(void     *p,
                     size_t   num_bytes);

int   NaCl_mprotect(void          *addr,
                    size_t        len,
                    int           prot) NACL_WUR;

int   NaCl_madvise(void           *start,
                   size_t         length,
                   int            advice) NACL_WUR;


#if NACL_LINUX
extern void *g_nacl_prereserved_sandbox_addr;
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*  NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_MEMORY_H_ */
