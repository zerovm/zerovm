/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * API for a set of generated decoder tables to use with NaClInstIter.
 * Allows both full and partial parsing, depending on what is defined
 * in the generated decoder tables.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_DECODE_TABLES_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_DECODE_TABLES_H__

#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/validator/x86/decoder/gen/ncopcode_prefix.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_decode_tables_types.h"
#include "native_client/src/trusted/validator/x86/x86_insts.h"

EXTERN_C_BEGIN

struct NaClOp;
struct NaClInst;
struct NaClInstNode;

/* The array used to look up instructions, based on matched prefix selector,
 * the the corresponding (first) opcode byte.
 */
typedef NaClOpcodeArrayOffset
NaclInstTableType[NaClInstPrefixEnumSize][NCDTABLESIZE];

/* Defines the values used to look up an instruction in
 * the opcode lookup table.
 */
typedef struct NaClPrefixOpcodeSelector {
  /* The starting offset in the opcode lookup table where
   * corresponding instructions are stored.
   */
  NaClPrefixOpcodeArrayOffset table_offset;
  /* The smallest opcode for which there is a table entry. */
  uint8_t first_opcode;
  /* The largest opcode for which there is a table entry. */
  uint8_t last_opcode;
} NaClPrefixOpcodeSelector;

/* Decoder tables used to decode instructions. */
typedef struct NaClDecodeTables {
  /* The table of operands. */
  const struct NaClOp* operands_table;
  /* The table of instructions. */
  const struct NaClInst* opcodes_table;
  /* The table of instructions defined for prefix/opcode entries. */
  const NaClOpcodeArrayOffset* opcode_entries;
  /* The table of prefix/opcode selectors. */
  const NaClPrefixOpcodeSelector* opcode_selectors;
  /* The definition of the undefined instruction. */
  const struct NaClInst* undefined;
  /* The look up table that returns the corresponding prefix mask
   * for the byte value, or zero if the byte doesn't define a valid
   * prefix byte.
   */
  const uint32_t* prefix_mask;
  /* The trie of hard coded instructions. */
  const struct NaClInstNode* hard_coded;
} NaClDecodeTables;

EXTERN_C_END

#endif /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_DECODE_TABLES_H__ */
