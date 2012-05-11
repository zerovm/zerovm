/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Defines code to detect trailing halts in a code segment, and how many
 * can be trimmed from the code segment without effecting the validator.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_HALT_TRIM_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_HALT_TRIM_H__

#include "src/validator/types_memory_model.h"

EXTERN_C_BEGIN

/* Takes a code memory segment, and returns the (safe) size to
 * use for validation. It does this by checking for trailing halt
 * instructions, and removes excess halts from the code segment memory.
 * This function assumes the code segment will be mapped to an aligned base
 * address.
 *
 * Parameters are:
 *   mbase - The beging of the memory segment to decode.
 *   sz - The number of bytes in the memory segment.
 *   alignment - The blocksize alignment used for the code segment.
 * Returns:
 *   The number of bytes in the trimmed memory segment.
 */
NaClMemorySize NCHaltTrimSize(uint8_t* mbase, NaClMemorySize sz,
                              uint8_t alignment);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_HALT_TRIM_H__ */
