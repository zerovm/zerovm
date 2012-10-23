/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/include/nacl_platform.h"
#include "src/service_runtime/sel_memory.h"
#include "src/service_runtime/sel_ldr.h"

void NaClMprotectGuards(struct NaClApp *nap)
{
  int err;
  uintptr_t start_addr = nap->mem_start;

  ZLOGS(LOG_DEBUG, "NULL detection region start 0x%08lx, size 0x%08x, end 0x%08lx",
      start_addr, NACL_SYSCALL_START_ADDR, start_addr + NACL_SYSCALL_START_ADDR);

  err = NaCl_mprotect((void *)start_addr, NACL_SYSCALL_START_ADDR, PROT_NONE);
  ZLOGFAIL(err != 0, err, FAILED_MSG);

  err = NaClVmmapAdd(&nap->mem_map, (start_addr - nap->mem_start) >> NACL_PAGESHIFT,
      NACL_SYSCALL_START_ADDR >> NACL_PAGESHIFT, PROT_NONE, (struct NaClMemObj *) NULL);
  ZLOGFAIL(err == 0, EFAULT, FAILED_MSG);

  /*
   * Now add additional guard pages for write protection.  We have 40G of address space
   * on either side of the main 4G address space that we have to make inaccessible..
   */
  err = NaCl_mprotect((void *)(start_addr - GUARDSIZE), GUARDSIZE, PROT_NONE);
  ZLOGFAIL(err != 0, err, FAILED_MSG);
  err = NaCl_mprotect((void *)(start_addr + FOURGIG), GUARDSIZE, PROT_NONE);
  ZLOGFAIL(err != 0, err, FAILED_MSG);
}
