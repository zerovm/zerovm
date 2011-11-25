/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Defines the user API to the state associated with matching instructions.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_STATE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_STATE_H_

#include "native_client/src/include/portability.h"
#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/validator/types_memory_model.h"

EXTERN_C_BEGIN

/* The meta model of an x86 opcode instruction. */
struct NaClInst;

/* The (user) representation of the parsed x86 instruction. */
struct NaClExpVector;

/* Defines the state used to match an instruction, while walking
 * instructions using the NaClInstIter.
 */
typedef struct NaClInstState NaClInstState;

struct NaClInst;

/* Returns the address (i.e. program counter) associated with the
 * currently matched instruction.
 */
NaClPcAddress NaClInstStateVpc(NaClInstState* state);

/* Given an iterator state, return the corresponding opcode (instruction)
 * that matches the currently matched instruction of the corresponding
 * instruction iterator.
 */
const struct NaClInst* NaClInstStateInst(NaClInstState* state);

/* Given an iterator state, return the corresponding expression tree
 * denoting the currently matched instruction of the corresponding
 * instruction iterator.
 */
struct NaClExpVector* NaClInstStateExpVector(NaClInstState* state);

/* Returns true if the instruction defined by the given state could
 * be decoded into a valid instruction.
 */
Bool NaClInstStateIsValid(NaClInstState* state);

/* Given an iterator state, return the number of bytes matched
 * by the currently matched instruction of the corresponding
 * instruction iterator.
 */
uint8_t NaClInstStateLength(NaClInstState* state);

/* Given an iterator state, return the index-th byte of the
 * currently matched instruction. Index must be less than
 * the value of the corresponding call to NaClInstStateLength.
 */
uint8_t NaClInstStateByte(NaClInstState* state, uint8_t index);

/* Returns the operand size (measured in bytes) of the instruction state. */
uint8_t NaClInstStateOperandSize(NaClInstState* state);

/* Returns the address size (measured in bits) of the instruction state. */
uint8_t NaClInstStateAddressSize(NaClInstState* state);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_STATE_H_ */
