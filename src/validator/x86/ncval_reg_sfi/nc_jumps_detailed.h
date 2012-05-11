/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_JUMPS_DETAILED_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_JUMPS_DETAILED_H__

/*
 * nc_jumps_detailed.h - Uses the jump sets collected by nc_jumps.h, and
 * walks the instructions a second time. Reports jump instructions that do
 * not validate and why.
 */

struct NaClValidatorState;
struct NaClInstIter;
struct NaClJumpSets;

/* Compares the collected actual jumps and the set of possible jump points,
 * and reports any descrepancies that don't follow NACL rules. Does this
 * by rewalking the instructions and finding instructions that don't follow NACL
 * rules.
 * Note: Replaces the post validator NaClJumpValidatorSummarize in nc_jumps.h,
 */
void NaClJumpValidatorSummarizeDetailed(struct NaClValidatorState* state);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_JUMPS_DETAILED_H__ */
