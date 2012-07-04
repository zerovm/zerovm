/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime memory allocation code
 */
#include <errno.h>
#include "include/nacl_platform.h"
#include "src/platform/nacl_log.h"
#include "src/service_runtime/nacl_config.h"

#define DISPATCH_THUNK_ADDRESS ((void*)0x5AFECA110000)

/*
 * When we're built into Chromium's "nacl_helper", its main will set this.
 */
void *g_nacl_prereserved_sandbox_addr = NULL;

void NaCl_page_free(void     *p,
                    size_t   size) {
  if (p == 0 || size == 0)
    return;
  if (munmap(p, size) != 0) {
    NaClLog(LOG_FATAL, "NaCl_page_free: munmap() failed");
  }
}

/*
 * NaCl_page_alloc_intern_flags
 */
static
int NaCl_page_alloc_intern_flags(void   **p,
                                 size_t size,
                                 int    map_flags) {
  void *addr;

  map_flags |= MAP_PRIVATE | MAP_ANONYMOUS;

  /* d'b: causes syntax error */
  /* todo(d'b): clear this */
//  NaClLog(4,
//            "sel_memory: NaCl_page_alloc_intern:"
//            " mmap(%p, %"NACL_PRIxS", %#x, %#x, %d, %"NACL_PRIdNACL_OFF64")\n",
//            *p, size, PROT_NONE, map_flags, -1,
//            (nacl_abi_off64_t) 0);
  addr = mmap(*p, size, PROT_NONE, map_flags, -1, (off_t) 0);
  if (MAP_FAILED == addr) {
    addr = NULL;
  }
  if (NULL != addr) {
    *p = addr;
  }
  return (NULL == addr) ? -ENOMEM : 0;
}

/*
 * Note that NaCl_page_alloc does not allocate pages that satisify
 * NaClIsAllocPageMultiple.  On linux/osx, the system does not impose
 * any such restrictions, and we only need to enforce the restriction
 * on NaCl app code to ensure that the app code is portable across all
 * host OSes.
 */
static
int NaCl_page_alloc_intern(void   **p,
                           size_t size) {
  int map_flags = 0;

  if (NULL != *p) {
    map_flags |= MAP_FIXED;
  }

  /*
   * Indicate to the kernel that we just want these pages allocated, not
   * committed.  This is important for systems with relatively little RAM+swap.
   * See bug 251.
   */
  map_flags |= MAP_NORESERVE;

  return NaCl_page_alloc_intern_flags(p, size, map_flags);
}

/*
 * Pick a "hint" address that is random.
 * update(d'b): for sake of determenism randomization has been disabled
 */
int NaCl_page_alloc_randomized(void **p, size_t size)
{
  *p = DISPATCH_THUNK_ADDRESS;
  NaClLog(LOG_INFO, "NaCl_page_alloc_randomized: hint 0x%"NACL_PRIxPTR"\n", (uintptr_t) *p);

  if(NaCl_page_alloc_intern_flags(p, size, 0) != 0)
    NaClLog(LOG_FATAL, "NaCl_page_alloc_randomized: failed, dropping hints\n");

  return 0;
}

int NaCl_page_alloc(void   **p,
                    size_t size) {
  void *addr = NULL;
  int rv;

  if (0 == (rv = NaCl_page_alloc_intern(&addr, size))) {
    *p = addr;
  }

  return rv;
}

/*
* This is critical to make the text region non-writable, and the data
* region read/write but no exec.  Of course, some kernels do not
* respect the lack of PROT_EXEC.
*/
int NaCl_mprotect(void          *addr,
                  size_t        len,
                  int           prot) {
  int  ret = mprotect(addr, len, prot);

  return ret == -1 ? -errno : ret;
}


int NaCl_madvise(void           *start,
                 size_t         length,
                 int            advice) {
  int ret = madvise(start, length, advice);

  /*
   * MADV_DONTNEED and MADV_NORMAL are needed
   */
  return ret == -1 ? -errno : ret;
}
