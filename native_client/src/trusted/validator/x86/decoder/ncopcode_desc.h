/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * ncopcode_desc.h - Descriptors to model opcode operands.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NCOPCODE_DESC_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NCOPCODE_DESC_H_

#include <stdio.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/shared/gio/gio.h"
#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/validator/x86/decoder/gen/ncopcode_prefix.h"
#include "native_client/src/trusted/validator/x86/decoder/gen/ncopcode_insts.h"
#include "native_client/src/trusted/validator/x86/decoder/gen/ncopcode_opcode_flags.h"
#include "native_client/src/trusted/validator/x86/decoder/gen/ncopcode_operand_kind.h"
#include "native_client/src/trusted/validator/x86/decoder/gen/ncopcode_operand_flag.h"
#include "native_client/src/trusted/validator/x86/x86_insts.h"

EXTERN_C_BEGIN

struct NaClDecodeTables;

/* Defines integer to represent sets of possible opcode (instruction) flags */
typedef uint64_t NaClIFlags;

/* Converts an NaClIFlagEnum to the corresponding bit in NaClIFlags. */
#define NACL_IFLAG(x) (((NaClIFlags) 1) << (x))

/* Models the empty set of opcode flags. */
#define NACL_EMPTY_IFLAGS ((NaClIFlags) 0)

/* Prints out the set of defined instruction flags. */
void NaClIFlagsPrint(struct Gio* out, NaClIFlags flags);

/* Defines integer to represent sets of possible operand flags. */
typedef uint32_t NaClOpFlags;

/* Converts an NaClOpFlag enum to the corresponding bit in NaClOpFlags. */
#define NACL_OPFLAG(x) (((NaClOpFlags) 1) << x)

/* Models the empty set of operand flags. */
#define NACL_EMPTY_OPFLAGS ((NaClOpFlags) 0)

/* Prints out the set of defined OPerand flags. */
void NaClOpFlagsPrint(struct Gio* out, NaClOpFlags flags);

/* Defines integer to represent sets of possible instruction disallow
 * flags.
 */
typedef uint16_t NaClDisallowsFlags;

/* Converts a NaClDisallowsFlag to the corresponding bit
 * in NaClDisallowsFlags.
 */
#define NACL_DISALLOWS_FLAG(x) (((NaClDisallowsFlags) 1) << (x))

/* Models the empty set of instruction disallows flags. */
#define NACL_EMPTY_DISALLOWS_FLAGS ((NaClDisallowsFlags) 0)

/* Metadata about an instruction operand. */
typedef struct NaClOp {
  /* The kind of the operand (i.e. kind of data modeled by the operand).*/
  NaClOpKind kind;
  /* Flags defining additional facts about the operand. */
  NaClOpFlags flags;
  /* Printing format string for operand. */
  const char* format_string;
} NaClOp;

/* Maxmimum number of opcode bytes per instruction. */
#define NACL_MAX_OPCODE_BYTES 3

/* Maximum number of opcode bytes used to model an instruction. Include
 * opcodes in the modrm byte, and register values encoded in the opcode.
 */
#define NACL_MAX_ALL_OPCODE_BYTES 4

/* Metadata about an instruction, defining a pattern. Note: Since the same
 * sequence of opcode bytes may define more than one pattern (depending on
 * other bytes in the parsed instruction), the patterns are
 * modeled using a singly linked list.
 */
typedef struct NaClInst {
  /* Defines the origin of this instruction. */
  NaClInstType insttype;
  /* Flags defining additional facts about the instruction. */
  NaClIFlags flags;
  /* The instruction that this instruction implements. */
  NaClMnemonic name;
  /* Defines opcode extentions, which encodes values for OpcodeInModRm,
   * OpcodePlusR, and OpcodeInModRmRm. Note: to fit the possible 9
   * bits of information in 8 bits, we assume that OpcodeInModRm
   * and OpcodePlusR do not happen in the same instruction.
   */
  uint8_t opcode_ext;
  /* The number of operands modeled for this instruction. */
  uint8_t num_operands;
  /* The corresponding models of the operands. */
  uint16_t operands_offset;
  /* Pointer to the next pattern to try and match for the
   * given sequence of opcode bytes.
   */
  uint16_t next_rule;
} NaClInst;

/* Returns the OpcodeInModRm value in the opcode_ext field. */
uint8_t NaClGetOpcodeInModRm(uint8_t opcode_ext);

/* Returns the OpcodeInModRmRm value in the opcode_ext field. */
uint8_t NaClGetOpcodeInModRmRm(uint8_t opcode_ext);

/* Returns the OpcodePlusR value in the opcode_ext field. */
uint8_t NaClGetOpcodePlusR(uint8_t opcode_ext);

/* Implements trie nodes for selecting instructions that must match
 * a specific sequence of bytes. Used to handle NOP cases.
 */
typedef struct NaClInstNode {
  /* The matching byte for the trie node. */
  const uint8_t matching_byte;
  /* The matching modeled instruction, if byte matched. */
  const uint16_t matching_inst;
  /* Node to match remaining bytes if matching_byte matches. */
  const struct NaClInstNode* success;
  /* Node to try next if match_byte doesn't match. Note:
   * The trie is generated in such a way that if the next input
   * byte is > matching_byte, no node in the fail subtree will
   * match the current input. That is, nodes in the trie are
   * sorted by the sequence of matching bytes.
   */
  const struct NaClInstNode* fail;
} NaClInstNode;

/* Returns the number of logical operands an instruction has. That is,
 * returns field num_operands unless the first operand is
 * a special encoding that extends the opcode.
 */
uint8_t NaClGetInstNumberOperands(const NaClInst* inst);

/* Returns the indexed logical operand for the instruction. That is,
 * returns the index-th operand unless the first operand is
 * a special encoding that extends the opcode. In the latter
 * case, the (index+1)-th operand is returned.
 */
const NaClOp* NaClGetInstOperand(const struct NaClDecodeTables* tables,
                                 const NaClInst* inst,
                                 uint8_t index);

/* Print out the given operand structure to the given file. */
void NaClOpPrint(struct Gio* f, const NaClOp* operand);

/* Returns a string defining bytes of the given prefix that are considered
 * prefix bytes, independent of the opcode.
 */
const char* OpcodePrefixBytes(NaClInstPrefix prefix);

/* Print out the given instruction to the given file. However, always
 * print the value NULL for next_rule, even if the value is non-null. This
 * function should be used to print out an individual opcode (instruction)
 * pattern.
 */
void NaClInstPrint(struct Gio* f,
                   const struct NaClDecodeTables* tables,
                   const NaClInst* inst);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NCOPCODE_DESC_H_ */
