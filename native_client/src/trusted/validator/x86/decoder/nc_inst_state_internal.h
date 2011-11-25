/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Internal implementation of the state associated with matching instructions.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_STATE_INTERNAL_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_STATE_INTERNAL_H_

#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/validator/x86/decoder/ncop_exps.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_state.h"
#include "native_client/src/trusted/validator/x86/ncinstbuffer.h"

EXTERN_C_BEGIN

/* The meta model of an x86 opcode instruction. */
struct NaClInst;

/* Model of a code segment. */
struct NaClSegment;

/* Decoder tables used to decode instructions. */
struct NaClDecodeTables;

/* Defines the type used to align OpExprNodes when memory allocating. */
typedef uint64_t NaClOpExpElement;

/* Defines the decoder tables to use to decode an instruction. */
struct NaClDecodeTables;

/* Model data needed to decode an x86 instruction. */
struct NaClInstState {
  /* The bytes used to parse the x86-32 instruction (may have added
   * zero filler if the instruction straddles the end of the memory segment).
   */
  NCInstBytes bytes;
  /* Define the (virtual pc) address associated with the instruction being
   * matched.
   */
  NaClPcAddress vpc;
  /* Define the upper limit on how many bytes can be in the instruction. */
  uint8_t length_limit;
  /* Define the number of prefix bytes processed. */
  uint8_t num_prefix_bytes; /* 0..4 */
  /* The prefix byte used to recognize the opcode, or zero if not applicable. */
  uint8_t opcode_prefix;
  /* Define the number of opcode bytes processed. */
  uint8_t num_opcode_bytes;
  /* If REX prefix found, its value. Otherwise zero. */
  uint8_t rexprefix;
  /* Number of REX prefix bytes found. */
  uint8_t num_rex_prefixes;
  /* If Mod/RM byte defined, its value. Otherwise zero. */
  uint8_t modrm;
  /* True if prefix bytes are duplicated prefix bytes. */
  Bool has_prefix_duplicates;
  /* True if prefix has ambiguous segment prefix bytes. */
  Bool has_ambig_segment_prefixes;
  /* True only if the instruction has an SIB byte. */
  Bool has_sib;
  /* If a SIB byte is defined, its value. Otherwise zero. */
  uint8_t sib;
  /* Define the number of displacement bytes matched by the instruction. */
  uint8_t num_disp_bytes;
  /* Define the index of the first displacement byte of the instruction, or
   * zero if num_disp_bytes == 0.
   */
  uint8_t first_disp_byte;
  /* Define the number of immediate bytes defined by the instruction. */
  uint8_t num_imm_bytes;
  /* Define the index of the first immediate byte of the instruction, or
   * zero if num_imm_bytes == 0;.
   */
  uint8_t first_imm_byte;
  /* Define the number of bytes to the second immediate value if defined
   * (defaults to zero).
   */
  uint8_t num_imm2_bytes;
  /* The computed (default) operand size associated with the instruction. */
  uint8_t operand_size;
  /* The computed (default) address size associated with the instruction. */
  uint8_t address_size;
  /* The set of prefix byte kinds associated with the instruction
   * (See kPrefixXXXX #define's in ncdecode.h)
   */
  uint32_t prefix_mask;
  /* The (opcode) instruction pattern used to match the instruction.
   * Note: If this value is NULL, we have not yet tried to match
   * the current instruction with the corresponding instruction iterator.
   * Note: One can assume that two instructions use the same modeled
   * NaClInst iff the pointers are equals.
   */
  const NaClInst* inst;
  /* The corresponding expression tree denoted by the matched instruction. */
  NaClExpVector nodes;
  /* Transient pointer, which is defined each time an instruction is decoded.
   * It defines the decoder tables to use to decode the instruction.
   */
  struct NaClDecodeTables* decoder_tables;
};

/* Model of an instruction iterator. */
struct NaClInstIter {
  /* Defines the decoder table to use to decode the instruction. */
  struct NaClDecodeTables* decoder_tables;
  /* Defines the segment to process */
  struct NaClSegment* segment;
  /* Defines the remaining memory to iterate over. */
  NCRemainingMemory memory;
  /* Defines the current (relative pc) index into the segment. */
  NaClMemorySize index;
  /* Defines the index of the current instruction, relative to
   * the beginning of the segment.
   */
  NaClMemorySize inst_count;
  /* The following fields define a ring buffer, where buffer_index
   * is the index of the current instruction in the buffer, and
   * buffer_size is the number of iterator states in the buffer.
   */
  size_t buffer_size;
  size_t buffer_index;
  struct NaClInstState* buffer;
};

/* Structure holding the results of consuming the opcode bytes of the
 * instruction.
 */
typedef struct {
  /* The applicable prefix byte selector, or 0 if no prefix selector. */
  uint8_t opcode_prefix;
  /* The (last) byte of the matched opcode. */
  uint8_t opcode_byte;
  /* The most specific prefix that the opcode bytes can match
   * (or OpcodePrefixEnumSize if no such patterns exist).
   */
  NaClInstPrefix matched_prefix;
  /* The number of bytes to subtract from the instruction length,
   * the next time GetNextNaClInstCandidates is called.
   */
  uint8_t next_length_adjustment;
} NaClInstPrefixDescriptor;

/* Given the current location of the (relative) pc of the given instruction
 * iterator, update the given state to hold the matched opcode
 * (instruction) pattern. If no matching pattern exists, set the state
 * to a matched undefined opcode (instruction) pattern. In all cases,
 * update the state to hold all information on the matched bytes of the
 * instruction.
 */
void NaClDecodeInst(struct NaClInstIter* iter, struct NaClInstState* state);

/* Returns the (undecoded) instruction state of the iterator. Should only
 * be used for testing.
 */

struct NaClInstState* NaClInstIterGetUndecodedState(struct NaClInstIter* iter);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_STATE_INTERNAL_H_ */
