/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <sys/mman.h>
#include "src/platform/portability.h"
#include "src/platform/sel_memory.h"
#include "src/loader/sel_ldr.h"

void NaClMprotectGuards(struct NaClApp *nap)
{
  int err;

  ZLOGS(LOG_DEBUG, "protecting bumpers");

  /*
   * make bumpers (guard pages) with "inaccessible" protection. the "left"
   * bumper size is 40gb + 64kb, the "right" one - 40gb
   */
  err = NaCl_mprotect((void *)(nap->mem_start - GUARDSIZE),
      GUARDSIZE + NACL_SYSCALL_START_ADDR, PROT_NONE);
  ZLOGFAIL(err != 0, err, FAILED_MSG);
  err = NaCl_mprotect((void *)(nap->mem_start + FOURGIG), GUARDSIZE, PROT_NONE);
  ZLOGFAIL(err != 0, err, FAILED_MSG);

  /* put information to the memory map */
  SET_MEM_MAP_IDX(nap->mem_map[LeftBumperIdx], "LeftBumper",
      nap->mem_start - GUARDSIZE, GUARDSIZE + NACL_SYSCALL_START_ADDR, PROT_NONE);
  SET_MEM_MAP_IDX(nap->mem_map[RightBumperIdx], "RightBumper",
      nap->mem_start + FOURGIG, GUARDSIZE, PROT_NONE);
}
