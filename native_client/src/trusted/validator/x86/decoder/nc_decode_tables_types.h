/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Defines types used in decoder tables.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NC_DECODE_TABLES_TYPES_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NC_DECODE_TABLES_TYPES_H__

#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

/* Defines the maximum allowable bytes per x86 instruction. */
#define NACL_MAX_BYTES_PER_X86_INSTRUCTION 15

/* Offset into generated table of operands, defining where the operands
 * are located.
 */
typedef uint16_t NaClOperandArrayOffset;

/* Offset into generated table of opcodes, defining where the instruction
 * is defined.
 */
typedef uint16_t NaClOpcodeArrayOffset;

/* Special value used to denote NULL for an NaClOpcodeArrayOffset value. */
#define NACL_OPCODE_NULL_OFFSET 0xFFFF

/* Offset into generated table of prefix opcode lookups. */
typedef uint16_t NaClPrefixOpcodeArrayOffset;

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NC_DECODE_TABLES_TYPES_H__ */
