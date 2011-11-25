/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * ncopcode_desc_inl.c - Holds inline routines for commonly used (simple)
 * functions in ncopcode_desc.h. Used to speed up code. Inlined routines
 * correspond to the following functions in ncopcode_desc.h, but with
 * an 'Inline' suffix:
 *
 *    NaClGetInstNumberOperands
 *    NaClGetInstOperand
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NCOPCODE_DESC_INL_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NCOPCODE_DESC_INL_H_

#include <assert.h>
#include "native_client/src/trusted/validator/x86/decoder/ncopcode_desc.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_decode_tables.h"

/* Returns the number of logical operands an instruction has. That is,
 * returns field num_operands unless the first operand is
 * a special encoding that extends the opcode.
 */
static uint8_t NaClGetInstNumberOperandsInline(const NaClInst* inst) {
  return inst->num_operands;
}

/* Returns the indexed logical operand for the instruction. That is,
 * returns the index-th operand unless the first operand is
 * a special encoding that extends the opcode. In the latter
 * case, the (index+1)-th operand is returned.
 */
static const INLINE NaClOp* NaClGetInstOperandInline(
    const NaClDecodeTables* tables,
    const NaClInst* inst, uint8_t index) {
  assert(index < inst->num_operands);
  return &tables->operands_table[inst->operands_offset + index];
}

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NCOPCODE_DESC_INL_H_ */
