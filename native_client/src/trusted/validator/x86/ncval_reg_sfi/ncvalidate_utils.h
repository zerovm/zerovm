/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Some useful utilities for validator patterns. */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATE_UTILS_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATE_UTILS_H__

#include "native_client/src/trusted/validator/x86/decoder/ncopcode_desc.h"

struct NaClInstState;
struct NaClExpVector;
struct NaClDecodeTables;

/* Special flag set to find set/use of an operand. */
extern const NaClOpFlags NaClOpSetOrUse;

/* Returns true if the instruction is a binary operation with
 * the given mnemonic name, and whose arguments are the given registers.
 *
 * Parameters:
 *   inst - The opcode corresponding to the instruction to check.
 *   name - The mnemonic name of the instruction.
 *   vector - The expression vector corresponding to the instruction to check.
 *   reg_1 - The register expected as the first argument
 *   reg_2 - The register expected as the second argument.
 */
Bool NaClIsBinaryUsingRegisters(const NaClInst* inst,
                                NaClMnemonic name,
                                struct NaClExpVector* vector,
                                NaClOpKind reg_1,
                                NaClOpKind reg_2);

/* Returns true if the instruction corresponds to a binary operation whose
 * result is put into REG_SET, and the resulst is computed using the values in
 * REG_SET and REG_USE.
 *
 * Parameters:
 *   inst - The opcode corresponding to the instruction to check.
 *   name - The mnemonic name of the binary operation.
 *   vector - The expression vector corresponding to the instruction to check.
 *   reg_set - The register set by the binary operation.
 *   reg_use - The register whose value is used (along with reg_set) to generate
 *             the or value.
 */
Bool NaClIsBinarySetUsingRegisters(const struct NaClDecodeTables* tables,
                                   const NaClInst* opcode,
                                   NaClMnemonic name,
                                   struct NaClExpVector* vector,
                                   NaClOpKind reg_1,
                                   NaClOpKind reg_2);

/* Returns true if the instruction corresponds to a move from
 * REG_USE to REG_SET.
 *
 * Parameters:
 *   inst - The opcode corresponding to the instruction to check.
 *   vector - The expression vector corresponding to the instruction to check.
 *   reg_set - The register set by the move.
 *   reg_use - The register whose value is used to define the set.
 */
Bool NaClIsMovUsingRegisters(const struct NaClDecodeTables* tables,
                             const NaClInst* inst,
                             struct NaClExpVector* vector,
                             NaClOpKind reg_set,
                             NaClOpKind reg_use);

/* Returns true if the given instruction's first operand corresponds to
 * a set of the register with the given name.
 *
 * Parameters:
 *   state - The instruction state to check.
 *   reg_name - The name of the register to check if set.
 */
Bool NaClOperandOneIsRegisterSet(struct NaClInstState* state,
                                 NaClOpKind reg_name);

/* Returns true if the given instruction's first operand corresponds to
 * a 32-bit value that is zero extended.
 *
 * Parameters:
 *   state - The instruction state to check.
 */
Bool NaClOperandOneZeroExtends(struct NaClInstState* state);

/* Returns true if the given instruction is binary where the first
 * Operand is a register set on the given register, and the
 * second operand corresponds to a 32-bit vlaue that is zero extended.
 */
Bool NaClAssignsRegisterWithZeroExtends(struct NaClInstState* state,
                                        NaClOpKind reg_name);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATE_UTILS_H__ */
