/*
 * Copyright 2010  The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_CONCURRENCY_OPS_H_
#define NATIVE_CLIENT_SRC_INCLUDE_CONCURRENCY_OPS_H_ 1


#include "include/nacl_base.h"
#include "include/portability.h"

static INLINE void NaClWriteMemoryBarrier() {
  __asm__ __volatile__("sfence");
}

static INLINE void NaClClearInstructionCache(void *start, void *end) {
  /*
   * Clearing the icache explicitly is not necessary on x86.  We could
   * call gcc's __builtin___clear_cache() on x86, where it is a no-op,
   * except that it is not available in Mac OS X's old version of gcc.
   */
  UNREFERENCED_PARAMETER(start);
  UNREFERENCED_PARAMETER(end);
}

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_CONCURRENCY_OPS_H_ */
