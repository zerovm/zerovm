/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/validator/x86/ncval_reg_sfi/ncvalidate_iter_detailed.h"
#include "src/validator/x86/ncval_reg_sfi/ncvalidate_iter_internal.h"

NaClValidatorState* NaClValidatorStateCreateDetailed(
    const NaClPcAddress vbase,
    const NaClMemorySize sz,
    const NaClOpKind base_register,
    const NaClCPUFeaturesX86* features) {
  NaClValidatorState* state =
      NaClValidatorStateCreate(vbase, sz, base_register, FALSE, features);
  if (state != NULL) {
    state->do_detailed = TRUE;
  }
  return state;
}
