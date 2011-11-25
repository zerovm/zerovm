/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATE_ITER_DETAILED_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATE_ITER_DETAILED_H__

/*
 * ncvalidate_iter_details.h: Secondary API for the validator to the
 * register-based sandbox.
 *
 * This is a secondary interface to the x86-64 validator for the register-based
 * sandbox. This version should be used when details about reporting errors
 * are needed. In particular, this interface should be used when the validator
 * is run in verbose or stubout mode. In these cases, getting details right
 * (i.e. the instruction that causes a code segment to violate NaCl rules)
 * is important. For verbose mode, this implies that the
 * error messages will report each instruction that violates a NaCl rule. For
 * stubout mode, it will automatically stub out (i.e. replace with HALT
 * instructions) instructions that violate NaCl rules.
 *
 * See ncvalidate_iter.h for the primary interface to the register-based sandbox
 * NaCl validator.
 *
 * This secondary interface is considerbly slower than the primary interface
 * in that it does two walks over the code segment instead of one. However, by
 * doing this second walk, it can generate more detailed error reports.
 * The secondary interface is engaged if one calls
 * NaClValidatorStateCreateDetailed  in place of NaClValidatorStateCreate.
 * The rest of the interface to the NaCl validator is the same.
 *
 * Basic usage:
 *   -- base is initial address of ELF file.
 *   -- limit is the size of the ELF file.
 *   -- maddr is the address to the memory of a section.
 *   -- vaddr is the starting virtual address associated with a section.
 *   -- size is the number of bytes in a section.
 *
 *   if (!NaClArchSupported()) fail;
 *   NaClValidatorState* state =
 *     NaClValidatorStateCreateDetailed(base, limit - base, 32, RegR15);
 *   if (state == NULL) fail;
 *   for each section:
 *     NaClValidateSegment(maddr, vaddr, size, state);
 *   if (!NaClValidatesOk(state)) fail;
 *   NaClValidatorStateDestroy(state);
 *
 * See the README file in this directory for more info on the general
 * structure of the validator.
 */

#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter.h"

/* Create a validator state to validate the code segment with the given
 * parameters, This state is set up to generate detailed errors instead
 * of summary (jump) errors.
 *
 * Note: Messages (if any) produced by the validator are sent to the stream
 * defined by native_client/src/shared/platform/nacl_log.h.
 *
 * Parameters.
 *   vbase - The virtual address for the contents of the code segment.
 *   sz - The number of bytes in the code segment
 *   alignment: 16 or 32, specifying alignment.
 *   base_register - OperandKind defining value for base register (or
 *     RegUnknown if not defined).
 * Returns:
 *   A pointer to an initialized validator state if everything is ok, NULL
 *   otherwise.
 */
NaClValidatorState* NaClValidatorStateCreateDetailed(
    const NaClPcAddress vbase,
    const NaClMemorySize sz,
    const uint8_t alignment,
    const NaClOpKind base_register);

/* Registers a detailed post validator to be used in place of the primary
 * (summary) post validator. Assumes the detailed post validator generates
 * error messages associated with instructions that violate validator
 * rules when the corresponding summary post validator only printed summary
 * information.
 */
void NaClRegisterDetailedPostValidator(
    NaClValidatorState *state,
    NaClValidatorPostValidate summary_post_validate,
    NaClValidatorPostValidate detailed_post_validate);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATE_ITER_DETAILED_H__ */
