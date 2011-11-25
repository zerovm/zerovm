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

#include "native_client/src/trusted/validator/types_memory_model.h"

EXTERN_C_BEGIN

/* Takes a code memory segment, and returns the (safe) size to
 * use for validation. It does this by checking for trailing halt
 * instructions, and removes excess halts from the code segment memory.
 *
 * Parameters are:
 *   mbase - The beging of the memory segment to decode.
 *   vbase - The (virtual) base address of the memory segment.
 *   alignment - The blocksize alignment used for the code segment.
 *   sz - The number of bytes in the memory segment. Updated to new
 *        appropriate new value if trimming occurs.
 *   limit - The address one byte past the last byte in the memory to
 *        be validdated. Updated to appropriate new value if trimming
 *        occurs.
 */
extern void NCHaltTrimSegment(uint8_t* mbase,
                              NaClPcAddress vbase,
                              uint8_t alignment,
                              NaClMemorySize *sz,
                              NaClPcAddress *vlimit);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_HALT_TRIM_H__ */
