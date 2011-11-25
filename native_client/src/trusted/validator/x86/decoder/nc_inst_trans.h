/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Defines the API to converting the recognized opcode (instruction),
 * in the instruction state, to the corresponding opcode expression.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_TRANS_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_TRANS_H_

#include "native_client/src/trusted/validator/x86/decoder/ncopcode_desc.h"

EXTERN_C_BEGIN

/* Defines the state used to match an instruction, while walking
 * instructions using the NaClInstIter.
 */
struct NaClInstState;

/* Constructs the corresponding NaClExpVector from the matched
 * NaClInst of the instruction state.
 */
void NaClBuildExpVector(struct NaClInstState* state);

/* Returns true iff the given 32 bit register is the base part of the
 * corresponding given 64-bit register.
 */
Bool NaClIs32To64RegPair(NaClOpKind reg32, NaClOpKind reg64);

/* Returns true iff the given (non-64 bit) subregister is a subpart
 * of the corresponding 64-bit register. Note: state is passed in
 * because different look ups are used for 8 bit registers, depending
 * on whether a REX prefix is found.
 */
Bool NaClIs64Subreg(struct NaClInstState* state,
                    NaClOpKind subreg, NaClOpKind reg64);

/* Given a 32-bit register, return the corresponding 64-bit register.
 * Returns RegUnknown if no such register exists.
 */
NaClOpKind NaClGet64For32BitReg(NaClOpKind reg32);

/* Given a 64-bit register, return the corresponding 32-bit register.
 * Returns RegUnknown if no such register exists.
 */
NaClOpKind NaClGet32For64BitReg(NaClOpKind reg64);

EXTERN_C_END

#endif   /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_TRANS_H_ */
