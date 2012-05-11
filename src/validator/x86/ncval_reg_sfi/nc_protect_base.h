/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_PROTECT_BASE_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_PROTECT_BASE_H__

#include "src/utils/types.h"
#include "src/validator/x86/decoder/gen/ncopcode_operand_kind.h"

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

/* The state associated with a decoded instruction. */
struct NaClInstState;

/* Defines locals used by the NaClBaseRegisterValidator to
 * record registers set in the current instruction, that are
 * a problem if not used correctly in the next instruction.
 */
typedef struct NaClRegisterLocals {
  /* Points to an instruction that contains an assignment to register ESP,
   * or NULL if the instruction doesn't set ESP. This is done so that we
   * can check if the next instruction uses the value of ESP to update RSP
   * (if not, we need to report that ESP is incorrectly assigned).
   */
  struct NaClInstState* esp_set_inst;
  /* Points to the instruction that contains an assignment to register EBP,
   * or NULL if the instruction doesn't set EBP. This is done so that we
   * can check if the next instruciton uses the value of EBP to update RBP
   * (if not, we need to report that EBP is incorrectly assigned).
   */
  struct NaClInstState* ebp_set_inst;
} NaClRegisterLocals;

/* Ths size of the circular buffer, used to keep track of registers
 * assigned in the previous instruction, that must be correctly used
 * in the current instruction, or reported as an error.
 */
#define NACL_REGISTER_LOCALS_BUFFER_SIZE 2

/* A circular buffer of two elements, used to  keep track of the
 * current/previous instruction.
 */
typedef struct NaClBaseRegisterLocals {
  NaClRegisterLocals buffer[NACL_REGISTER_LOCALS_BUFFER_SIZE];
  int previous_index;
  int current_index;
} NaClBaseRegisterLocals;

/* Initializes memory to hold local information for validator
 * NaClBaseRegisterValidator. Returns true if successful.
 */
void NaClBaseRegisterMemoryInitialize(struct NaClValidatorState* state);

/* Validator function to check that the base register is never set. */
void NaClBaseRegisterValidator(struct NaClValidatorState* state);


/* Post iteration validator summarization function. */
void NaClBaseRegisterSummarize(struct NaClValidatorState* state);

/* Checks for pattern
 *     op %reg32), ...
 *     lea %reg64, [%reg64+%rbase*1]
 *
 * where reg64 is the passed 64-bit register, reg32 is the
 * corresponding 32-bit register, and op is a 32-bit zero-extending
 * operation (such as mov).
 */
Bool NaClAcceptLeaWithMoveLea32To64(struct NaClValidatorState* state,
                                    NaClOpKind reg);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_PROTECT_BASE_H__ */
