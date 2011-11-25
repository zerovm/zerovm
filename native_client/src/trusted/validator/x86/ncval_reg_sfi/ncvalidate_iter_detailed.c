/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter_detailed.h"

#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter_internal.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidator_registry.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidator_registry_detailed.h"

void NaClRegisterDetailedPostValidator(
    NaClValidatorState *state,
    NaClValidatorPostValidate summary_post_validate,
    NaClValidatorPostValidate detailed_post_validate) {
  int i;
  for (i = 0; i < state->number_validators; ++i) {
    NaClValidatorDefinition *defn = &state->validators[i];
    if (defn->post_validate == summary_post_validate) {
      defn->post_validate = detailed_post_validate;
    }
  }
}

static void NaClValidatorDetailedRulesInit(NaClValidatorState* state) {
  /* First add primary validator routines. */
  NaClValidatorRulesInit(state);
  /* Now add detailed post validators. */
  NaClAddDetailedPostValidators(state);
}

NaClValidatorState* NaClValidatorStateCreateDetailed(
    const NaClPcAddress vbase,
    const NaClMemorySize sz,
    const uint8_t alignment,
    const NaClOpKind base_register) {
  NaClValidatorState* state =
      NaClValidatorStateCreate(vbase, sz, alignment, base_register);
  /* Override rules initialize function to add appropriate post validators. */
  state->rules_init_fn = NaClValidatorDetailedRulesInit;
  return state;
}
