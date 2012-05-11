/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_JUMPS_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_JUMPS_H__

/*
 * nc_jumps.h - Implements set of possible jump points, and set of
 * actual jump points, and the verification that the possible
 * (explicit) jumps only apply to valid actual jumps.
 */

#include <stdio.h>

#include "src/utils/types.h"
#include "src/validator/x86/ncval_reg_sfi/address_sets.h"

/* The model of a validator state. */
struct NaClValidatorState;

/* The model of an iterator through instructions in a code segment. */
struct NaClInstIter;

/* The model of a parsed instruction. */
struct NaClInstState;

/* Holds information collected about each instruction, and the
 * targets of possible jumps. Then, after the code has been processed,
 * this information is processed to see if there are any invalid jumps.
 */
typedef struct NaClJumpSets {
  /* Holds the set of possible target addresses that can be the result of
   * a jump.
   */
  NaClAddressSet actual_targets;
  /* Holds the set of valid instruction entry points (whenever a pattern of
   * multiple instructions are used, the sequence will be treated as atomic,
   * only having the first address in the set).
   */
  NaClAddressSet possible_targets;
  /* Removed targets, due to instruction being in the middle of an atomic
   * sequence. Note: This is needed so that we can allow validators to
   * run in any order. If we didn't do this, then we are very timing dependent
   * on calls to NaClMarkInstructionJumpIllegal, which must appear after
   * the call to NaClJumpValidator.
   */
  NaClAddressSet removed_targets;
  /* Holds the (array) size of each set above. */
  size_t set_array_size;
} NaClJumpSets;

/* When true, changes the behaviour of NcAddJump to use mask 0xFF for
 * indirect jumps (which is a nop). This allows performance tests for
 * compiled libraries without having to hand tweak the source code.
 */
extern Bool NACL_FLAGS_identity_mask;

/* Initializes jump sets to track the set of possible and actual (explicit)
 * address. Returns true if successful.
 */
Bool NaClJumpValidatorInitialize(struct NaClValidatorState* state);

/* Collects information on instruction addresses, and where explicit jumps
 * go to.
 */
void NaClJumpValidator(struct NaClValidatorState* state);

/* Don't record anything but the instruction address, in order to validate
 * basic block alignment at the end of validation.
 */
void NaClJumpValidatorRememberIpOnly(struct NaClValidatorState* state);

/* Compares the collected actual jumps and the set of possible jump points,
 * and reports any descrepancies that don't follow NACL rules.
 */
void NaClJumpValidatorSummarize(struct NaClValidatorState* state);

/* Cleans up memory used by the jump validator. */
void NaClJumpValidatorCleanUp(struct NaClValidatorState* state);

/* Record that the given instruction can't be a possible target of a jump,
 * because it appears as the non-first
 * instruciton in a NACL pattern. This should be called on all such non-first
 * instructions (for NACL patterns) so that the instuction sequence is
 * checked to be atomic.
 */
void NaClMarkInstructionJumpIllegal(struct NaClValidatorState* state,
                                    struct NaClInstState* inst);

/* Records that the given sequence of distance instructions (starting with the
 * current instruction, and proceeding backwards) can't be a possible target
 * of a jump, because they appear as the non-first instruction of a NACL
 * pattern.
 */
void NaClMarkInstructionsJumpRangeIllegal(struct NaClValidatorState* vstate,
                                          int distance);

/* Same as NaClMarkInstructionJumpIllegal, except that it marks the
 * n-th instruction back from the current instruction.
 */
void NaClMarkInstructionJumpIllegalLookback(
    struct NaClInstIter* iter,
    struct NaClValidatorState* state,
    size_t n);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_JUMPS_H__ */
