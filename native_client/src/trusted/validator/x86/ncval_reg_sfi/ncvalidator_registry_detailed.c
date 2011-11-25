/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidator_registry_detailed.h"

#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter_detailed.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_jumps.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_jumps_detailed.h"

void NaClAddDetailedPostValidators(struct NaClValidatorState* state) {
  NaClRegisterDetailedPostValidator(
      state,
      (NaClValidatorPostValidate) NaClJumpValidatorSummarize,
      (NaClValidatorPostValidate) NaClJumpValidatorSummarizeDetailed);
}
