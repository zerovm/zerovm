/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/include/nacl_platform.h"
#include "src/service_runtime/sel_memory.h"
#include "src/service_runtime/sel_ldr.h"

#define FOURGIG     (((size_t) 1) << 32)
#define GUARDSIZE   (10 * FOURGIG)

NaClErrorCode NaClMprotectGuards(struct NaClApp *nap) {
  uintptr_t start_addr;
  int       err;

  start_addr = nap->mem_start;

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
  return LOAD_OK;
}
