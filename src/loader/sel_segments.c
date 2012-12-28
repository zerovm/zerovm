/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Runtime.
 */
#include "src/loader/sel_rt.h"

nacl_reg_t NaClGetStackPtr(void) {
  nacl_reg_t rsp;

  __asm__("mov %%rsp, %0" : "=r" (rsp) : );
  return rsp;
}
