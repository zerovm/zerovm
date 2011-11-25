/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_MEMORY_PROTECT_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_MEMORY_PROTECT_H__

/* nc_memory_protect.h - For 64-bit mode, verifies that we don't access
 * memory that is out of range.
 */

#include "native_client/src/shared/utils/types.h"

/*
 * Note: The function NcMemoryReferenceValidator is used as a validator
 * function to be applied to a validated segment, as defined in
 * ncvalidate_iter.h.
 */

struct NaClValidatorState;
struct NaClInstIter;

/*
 * When true, check both uses and sets of memory. When false, only
 * check sets.
 */
extern Bool NACL_FLAGS_read_sandbox;

/*
 * Verifies that we don't access memory store with an out of range
 * address. That implies that when storing (or reading when doing read
 * sandboxing), in a memory offset of the form:
 *
 *     [base + index * scale + displacement]
 *
 * (1) base is either the reserved base register (r15), or rsp, or rbp.
 *
 * (2) Either the index isn't defined, or the index is a 32-bit register and
 *     the previous instruction must assign a value to index that is 32-bits
 *     with zero extension.
 *
 * (3) The displacement can't be larger than 32 bits.
 *
 * SPECIAL CASE: We allow all stores of the form [%rip + displacement].
 *
 * NOTE: in x86 code, displacements can't be larger than 32 bits.
 */
void NaClMemoryReferenceValidator(struct NaClValidatorState* state,
                                  struct NaClInstIter* iter,
                                  void* ignore);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_MEMORY_PROTECT_H__ */
