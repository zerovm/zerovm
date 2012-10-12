/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime memory allocation code
 */
#include "src/include/nacl_platform.h"
#include "src/platform/nacl_log.h"

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
 * d'b: made global function
 */
int NaCl_page_alloc_intern_flags(void **p, size_t size, int map_flags)
{
  void *addr;

  map_flags |= MAP_PRIVATE | MAP_ANONYMOUS;

  /* d'b: syntax error fixed */
  NaClLog(LOG_DEBUG, "sel_memory: NaCl_page_alloc_intern: mmap(%p, %lX, %#x, %#x, %d, %d)\n",
      *p, size, PROT_NONE, map_flags, -1, 0);

  addr = mmap(*p, size, PROT_NONE, map_flags, -1, (off_t)0);
  if(MAP_FAILED == addr) addr = NULL;
  if(NULL != addr) *p = addr;
  return (NULL == addr) ? -ENOMEM : 0;
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
