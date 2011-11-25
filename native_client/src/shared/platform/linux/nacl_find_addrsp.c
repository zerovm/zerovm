/*
 * Copyright 2010 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "native_client/src/shared/platform/nacl_find_addrsp.h"

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/nacl_log.h"

/* bool */
int NaClFindAddressSpace(uintptr_t *addr, size_t memory_size) {
  void *map_addr;

  NaClLog(4,
          "NaClFindAddressSpace: looking for %"NACL_PRIxS" bytes\n",
          memory_size);
  map_addr = mmap(NULL, memory_size,
                  PROT_READ | PROT_WRITE,
                  MAP_ANONYMOUS | MAP_NORESERVE | MAP_PRIVATE,
                  0,
                  0);
  if (MAP_FAILED == map_addr) {
    return 0;
  }
  NaClLog(4,
          "NaClFindAddressSpace: got addr %"NACL_PRIxPTR"\n",
          (uintptr_t) map_addr);
  *addr = (uintptr_t) map_addr;
  return 1;
}
