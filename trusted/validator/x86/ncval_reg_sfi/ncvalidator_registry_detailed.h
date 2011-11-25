/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Define the validator registry that force specific detailed validator rules
 * to be applied.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATOR_REGISTTRY_DETAILED_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATOR_REGISTTRY_DETAILED_H__

struct NaClValidatorState;

/* A routine that installs detailed post validators which replace the
 * corresponding summary post validators used by the primary library
 * interface to the register-based SFI sandbox.
 */
void NaClAddDetailedPostValidators(struct NaClValidatorState* state);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATOR_REGISTTRY_DETAILED_H__ */
