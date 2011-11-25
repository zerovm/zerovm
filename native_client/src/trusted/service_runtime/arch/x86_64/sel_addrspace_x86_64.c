/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdio.h>

#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

#define FOURGIG     (((size_t) 1) << 32)
#define GUARDSIZE   (10 * FOURGIG)

NaClErrorCode NaClMprotectGuards(struct NaClApp *nap) {
  uintptr_t start_addr;
  int       err;
  void      *guard[2];

  start_addr = nap->mem_start;
  guard[0] = (void *)(start_addr - GUARDSIZE);
  guard[1] = (void *)(start_addr + FOURGIG);

  NaClLog(3,
          ("NULL detection region start 0x%08"NACL_PRIxPTR", "
           "size 0x%08x, end 0x%08"NACL_PRIxPTR"\n"),
          start_addr, NACL_SYSCALL_START_ADDR,
          start_addr + NACL_SYSCALL_START_ADDR);
  if ((err = NaCl_mprotect((void *) start_addr,
                           NACL_SYSCALL_START_ADDR,
                           PROT_NONE)) != 0) {
    NaClLog(LOG_ERROR,
            ("NaClMprotectGuards:"
             " NaCl_mprotect(0x%016"NACL_PRIxPTR", 0x%016x, 0x%x) failed,"
             " error %d (NULL pointer guard page)\n"),
            start_addr, NACL_SYSCALL_START_ADDR, PROT_NONE,
            err);
    return LOAD_MPROTECT_FAIL;
  }
  if (!NaClVmmapAdd(&nap->mem_map,
                    (start_addr - nap->mem_start) >> NACL_PAGESHIFT,
                    NACL_SYSCALL_START_ADDR >> NACL_PAGESHIFT,
                    PROT_NONE,
                    (struct NaClMemObj *) NULL)) {
    NaClLog(LOG_ERROR, ("NaClMprotectGuards: NaClVmmapAdd failed"
                        " (NULL pointer guard page)\n"));
    return LOAD_MPROTECT_FAIL;
  }

  /*
   * Now add additional guard pages for write protection.  We have 40G
   * of address space on either side of the main 4G address space that
   * we have to make inaccessible....
   */
#if !NACL_WINDOWS
  /*
   * On Windows we do not alter the page protection of the guard regions,
   * as this would require them to be committed. Instead we keep them
   * reserved but uncommitted, which means that access attempts will fault.
   * On other systems, we mprotect the guards.
   */
  if ((err = NaCl_mprotect((void *) (start_addr - GUARDSIZE),
                           GUARDSIZE,
                           PROT_NONE)) != 0) {
    NaClLog(LOG_ERROR,
            ("NaClMprotectGuards: "
             "NaCl_mprotect(0x%016"NACL_PRIxPTR", "
             "0x%016"NACL_PRIxS", 0x%x) failed."
             " error %d (pre-address-space guard)\n"),
            (start_addr - GUARDSIZE),
            GUARDSIZE,
            PROT_NONE,
            err);
    return LOAD_MPROTECT_FAIL;
  }
  if ((err = NaCl_mprotect((void *) (start_addr + FOURGIG),
                           GUARDSIZE,
                           PROT_NONE)) != 0) {
    NaClLog(LOG_ERROR,
            ("NaClMprotectGuards:"
             " NaCl_mprotect(0x%016"NACL_PRIxPTR", "
             "0x%016"NACL_PRIxS", 0x%x) failed."
             " error %d (post-address-space guard)\n"),
            (start_addr + FOURGIG),
            GUARDSIZE,
            PROT_NONE,
            err);
    return LOAD_MPROTECT_FAIL;
  }
#endif
  return LOAD_OK;
}
