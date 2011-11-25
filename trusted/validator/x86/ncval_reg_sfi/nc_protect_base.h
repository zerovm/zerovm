/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_PROTECT_BASE_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_PROTECT_BASE_H__

#include <stdio.h>

/* nc_protect_base.h - For 64-bit mode, verifies that no instruction
 * changes the value of the base register.
 */

/*
 * Note: The function BaseRegisterValidator is used as a validator
 * function to be applied to a validated segment, as defined in
 * ncvalidate_iter.h.
 */

/* The model of a validator state. */
struct NaClValidatorState;

/* The model of an iterator through instructions in a code segment. */
struct NaClInstIter;

/* Defines a data structure that holds data local to function
 * the validator function NaClBaseRegisterValidator.
 */
struct NaClBaseRegisterLocals;

/* Create memory to hold local information for validator
 * NaClBaseRegisterValidator.
 */
struct NaClBaseRegisterLocals* NaClBaseRegisterMemoryCreate(
    struct NaClValidatorState* state);

/* Create memory to hold local information for validator
 * NaClBaseRegisterValidator.
 */
void NaClBaseRegisterMemoryDestroy(struct NaClValidatorState*state,
                                   struct NaClBaseRegisterLocals* locals);

/* Validator function to check that the base register is never set. */
void NaClBaseRegisterValidator(struct NaClValidatorState* state,
                               struct NaClInstIter* iter,
                               struct NaClBaseRegisterLocals* locals);

/* Post iteration validator summarization function. */
void NaClBaseRegisterSummarize(struct NaClValidatorState* state,
                               struct NaClInstIter* iter,
                               struct NaClBaseRegisterLocals* locals);


#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_PROTECT_BASE_H__ */
