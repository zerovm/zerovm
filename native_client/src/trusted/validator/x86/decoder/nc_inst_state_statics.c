/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file contains includes, static functions, and constants that are used
 * in nc_inst_state.c, but have been factored out and put into this file, so
 * that we can test them. That is, to allow nc_inst_state.c and
 * nc_inst_state_Tests.cc to use them.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_STATE_STATICS_C__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_STATE_STATICS_C__

#include <stdio.h>
#include <assert.h>
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/utils/debugging.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_decode_tables.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_iter.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_state_internal.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_trans.h"
#include "native_client/src/trusted/validator/x86/decoder/ncop_exps.h"
#include "native_client/src/trusted/validator/x86/decoder/ncopcode_desc.h"
#include "native_client/src/trusted/validator/x86/nc_segment.h"

#include "native_client/src/trusted/validator/x86/ncinstbuffer_inl.c"
#include "native_client/src/trusted/validator/x86/x86_insts_inl.c"

EXTERN_C_BEGIN

/* Given the current location of the instruction iterator, initialize
 * the given state (to match).
 */
static void NaClInstStateInit(NaClInstIter* iter, NaClInstState* state) {
  NaClMemorySize limit;
  NCInstBytesInitInline(&state->bytes);
  state->decoder_tables = iter->decoder_tables;
  state->vpc = iter->segment->vbase + iter->index;
  limit = iter->segment->size - iter->index;
  if (limit > NACL_MAX_BYTES_PER_X86_INSTRUCTION) {
    limit = NACL_MAX_BYTES_PER_X86_INSTRUCTION;
  }
  state->length_limit = (uint8_t) limit;
  DEBUG(NaClLog(LOG_INFO,
                "length limit = %"NACL_PRIu8"\n", state->length_limit));
  state->num_prefix_bytes = 0;
  state->opcode_prefix = 0;
  state->num_opcode_bytes = 0;
  state->rexprefix = 0;
  state->num_rex_prefixes = 0;
  state->modrm = 0;
  state->has_prefix_duplicates = FALSE;
  state->has_ambig_segment_prefixes = FALSE;
  state->has_sib = FALSE;
  state->sib = 0;
  state->num_disp_bytes = 0;
  state->first_disp_byte = 0;
  state->num_imm_bytes = 0;
  state->first_imm_byte = 0;
  state->num_imm2_bytes = 0;
  state->prefix_mask = 0;
  state->inst = NULL;
  state->nodes.is_defined = FALSE;
  state->nodes.number_expr_nodes = 0;
}

/* Computes the number of bytes defined for operands of the matched
 * instruction of the given state.
 */
static int NaClExtractOpSize(NaClInstState* state) {
  if (NaClHasBit(state->inst->flags, NACL_IFLAG(OperandSize_b))) {
    return 1;
  }
  if (NACL_TARGET_SUBARCH == 64) {
    if ((state->rexprefix && NaClRexW(state->rexprefix)) ||
        (NaClHasBit(state->inst->flags, NACL_IFLAG(OperandSizeForce64)))) {
      return 8;
    }
  }
  if (NaClHasBit(state->prefix_mask, kPrefixDATA16) &&
      (NACL_EMPTY_IFLAGS ==
       (state->inst->flags & NACL_IFLAG(SizeIgnoresData16)))) {
    return 2;
  }
  if ((NACL_TARGET_SUBARCH == 64) &&
      NaClHasBit(state->inst->flags, NACL_IFLAG(OperandSizeDefaultIs64))) {
    return 8;
  }
  return 4;
}

/* Computes the number of bits defined for addresses of the matched
 * instruction of the given state.
 */
static int NaClExtractAddressSize(NaClInstState* state) {
  if (NACL_TARGET_SUBARCH == 64) {
    return NaClHasBit(state->prefix_mask, kPrefixADDR16) ? 32 : 64;
  } else {
    return NaClHasBit(state->prefix_mask, kPrefixADDR16) ? 16 : 32;
  }
}

/* Manual implies only 4 bytes is allowed, but I have found up to 6.
 * Why don't we allow any number, so long as (1) There is room for
 * at least one opcode byte, and (2) we don't exceed the max bytes.
 */
static const int kNaClMaximumPrefixBytes =
    NACL_MAX_BYTES_PER_X86_INSTRUCTION - 1;

/* Captures ambiguous segment prefix forms. Used to make
 * detection of multiple prefix segment bytes.
 */
static const uint32_t segment_prefix_forms =
    kPrefixSEGCS | kPrefixSEGSS | kPrefixSEGFS |
    kPrefixSEGGS | kPrefixSEGES | kPrefixSEGDS;

/* Match any prefix bytes that can be associated with the instruction
 * currently being matched.
 */
static Bool NaClConsumePrefixBytes(NaClInstState* state) {
  uint8_t next_byte;
  int i;
  uint32_t prefix_form;
  for (i = 0; i < kNaClMaximumPrefixBytes; ++i) {
    /* Quit early if no more bytes in segment. */
    if (state->bytes.length >= state->length_limit) break;

    /* Look up the corresponding prefix bit associated
     * with the next byte in the segment, and record it.
     */
    next_byte = NCRemainingMemoryLookaheadInline(state->bytes.memory,0);
    prefix_form = state->decoder_tables->prefix_mask[next_byte];
    if (prefix_form == 0) break;
    next_byte = NCInstBytesReadInline(&state->bytes);
    DEBUG(NaClLog(LOG_INFO,
                  "Consume prefix[%d]: %02"NACL_PRIx8" => %"NACL_PRIx32"\n",
                  i, next_byte, prefix_form));
    /* Before updating prefix mask, determine if the prefix byte is
     * a duplicate.
     */
    if ((state->prefix_mask & prefix_form)) {
      state->has_prefix_duplicates = TRUE;
      DEBUG(NaClLog(LOG_INFO,
                    "duplicate prefix %02"NACL_PRIx8" detected.\n", next_byte));
    } else if ((prefix_form & segment_prefix_forms) &&
               (state->prefix_mask & segment_prefix_forms)) {
      state->has_ambig_segment_prefixes = TRUE;
      DEBUG(NaClLog(LOG_INFO,
                    "ambiguos segment prefix %02"NACL_PRIx8" detected.\n",
                    next_byte));
    }
    state->prefix_mask |= prefix_form;
    ++state->num_prefix_bytes;
    DEBUG(NaClLog(LOG_INFO,
                  "  prefix mask: %08"NACL_PRIx32"\n", state->prefix_mask));

    /* If the prefix byte is a REX prefix, record its value, since
     * bits 5-8 of this prefix bit may be needed later.
     */
    if ((NACL_TARGET_SUBARCH == 64) && prefix_form == kPrefixREX) {
      state->rexprefix = next_byte;
      DEBUG(NaClLog(LOG_INFO,
                    "  rexprefix = %02"NACL_PRIx8"\n", state->rexprefix));
      ++state->num_rex_prefixes;
    }
  }
  return TRUE;
}

/* Assuming we have matched the byte sequence OF 38, consume the corresponding
 * following (instruction) opcode byte, returning the most specific prefix the
 * patterns can match (or NaClInstPrefixEnumSize if no such patterns exist);
 */
static void NaClConsume0F38XXNaClInstBytes(NaClInstState* state,
                                           NaClInstPrefixDescriptor* desc) {
  /* Fail if there are no more bytes. Otherwise, read the next
   * byte.
   */
  if (state->bytes.length >= state->length_limit) {
    desc->matched_prefix = NaClInstPrefixEnumSize;
    return;
  }

  desc->opcode_byte = NCInstBytesReadInline(&state->bytes);
  DEBUG(NaClLog(LOG_INFO, "Consume inst byte %02"NACL_PRIx8".\n",
                desc->opcode_byte));
  if (NaClExcludesBit(state->prefix_mask, kPrefixREP)) {
    if (NaClHasBit(state->prefix_mask, kPrefixREPNE)) {
      /* Note: Flag OpcodeAllowsData16 will explicitly clarify
       * ambigous case of both REP and DATA16 prefixes.
       */
      desc->matched_prefix = PrefixF20F38;
      desc->opcode_prefix = kValueREPNE;
    } else if (NaClHasBit(state->prefix_mask, kPrefixDATA16)) {
      desc->matched_prefix = Prefix660F38;
      desc->opcode_prefix = kValueDATA16;
    } else {
      desc->matched_prefix = Prefix0F38;
    }
    return;
  }
  /* If reached, can't match special prefixes, fail. */
  desc->matched_prefix = NaClInstPrefixEnumSize;
}

/* Assuming we have matched the byte sequence OF 3A, consume the corresponding
 * following (instruction) opcode byte, returning the most specific prefix the
 * patterns can match (or NaClInstPrefixEnumSize if no such patterns exist).
 */
static void NaClConsume0F3AXXNaClInstBytes(NaClInstState* state,
                                           NaClInstPrefixDescriptor* desc) {
  /* Fail if there are no more bytes. Otherwise, read the next
   * byte and choose appropriate prefix.
   */
  if (state->bytes.length >= state->length_limit) {
    desc->matched_prefix = NaClInstPrefixEnumSize;
    return;
  }

  desc->opcode_byte = NCInstBytesReadInline(&state->bytes);
  DEBUG(NaClLog(LOG_INFO, "Consume inst byte %02"NACL_PRIx8".\n",
                desc->opcode_byte));
  if (NaClExcludesBit(state->prefix_mask, kPrefixREP) &&
      NaClExcludesBit(state->prefix_mask, kPrefixREPNE)) {
    if (NaClHasBit(state->prefix_mask, kPrefixDATA16)) {
      desc->matched_prefix = Prefix660F3A;
      desc->opcode_prefix = kValueDATA16;
    } else {
      desc->matched_prefix = Prefix0F3A;
    }
    return;
  }
  /* If reached, can't match special prefixes, fail. */
  desc->matched_prefix = NaClInstPrefixEnumSize;
}

/* Assuming we have matched byte OF, consume the corresponding
 * following (instruction) opcode byte, returning the most specific
 * prefix the patterns can match (or NaClInstPrefixEnumSize if no such
 * patterns exist).
 */
static void NaClConsume0FXXNaClInstBytes(NaClInstState* state,
                                         NaClInstPrefixDescriptor* desc) {
  if (NaClHasBit(state->prefix_mask, kPrefixREPNE)) {
    if (NaClExcludesBit(state->prefix_mask, kPrefixREP)) {
      /* Note: Flag OpcodeAllowsData16 will explicitly clarify
       * ambigous case of both REPNE and DATA16 prefixes.
       */
      desc->matched_prefix = PrefixF20F;
      desc->opcode_prefix = kValueREPNE;
      return;
    }
  } else {
    if (NaClHasBit(state->prefix_mask, kPrefixREP)) {
      /* Note: Flag OpcodeAllowsData16 will explicitly clarify
       * ambigous case of both REP and DATA16 prefixes.
       */
      desc->matched_prefix = PrefixF30F;
      desc->opcode_prefix = kValueREP;
    } else if (NaClHasBit(state->prefix_mask, kPrefixDATA16)) {
      desc->matched_prefix = Prefix660F;
      desc->opcode_prefix = kValueDATA16;
    } else {
      desc->matched_prefix = Prefix0F;
    }
    return;
  }
  /* If reached, can't match special prefixes, fail. */
  desc->matched_prefix = NaClInstPrefixEnumSize;
}

/* Consume one of the x87 instructions that begin with D8-Df, and
 * match the most specific prefix pattern the opcode bytes can match.
 */
static void NaClConsumeX87NaClInstBytes(NaClInstState* state,
                                        NaClInstPrefixDescriptor* desc) {
  if (state->bytes.length < state->length_limit) {
    /* Can be two byte opcode. */
    desc->matched_prefix =
        (NaClInstPrefix) (PrefixD8 +
                          (((unsigned) desc->opcode_byte) - 0xD8));
    desc->opcode_byte = NCInstBytesReadInline(&state->bytes);
    DEBUG(NaClLog(LOG_INFO, "Consume inst byte %02"NACL_PRIx8".\n",
                  desc->opcode_byte));
    return;
  }

  /* If reached, can only be single byte opcode, match as such. */
  desc->matched_prefix = NoPrefix;
}

/* Consume the opcode bytes, and return the most specific prefix pattern
 * the opcode bytes can match (or NaClInstPrefixEnumSize if no such pattern
 * exists).
 */
static void NaClConsumeInstBytes(NaClInstState* state,
                                 NaClInstPrefixDescriptor* desc) {

  /* Initialize descriptor to the fail state. */
  desc->opcode_prefix = 0;
  desc->opcode_byte = 0x0;
  desc->matched_prefix = NaClInstPrefixEnumSize;
  desc->next_length_adjustment = 0;

  /* Be sure that we don't exceed the segment length. */
  if (state->bytes.length >= state->length_limit) return;

  desc->opcode_byte = NCInstBytesReadInline(&state->bytes);
  DEBUG(NaClLog(LOG_INFO, "Consume inst byte %02"NACL_PRIx8".\n",
                desc->opcode_byte));
  switch (desc->opcode_byte) {
    case 0x0F:
      if (state->bytes.length >= state->length_limit) return;
      desc->opcode_byte = NCInstBytesReadInline(&state->bytes);
      DEBUG(NaClLog(LOG_INFO, "Consume inst byte %02"NACL_PRIx8".\n",
                    desc->opcode_byte));
      switch (desc->opcode_byte) {
        case 0x38:
          NaClConsume0F38XXNaClInstBytes(state, desc);
          break;
        case 0x3a:
          NaClConsume0F3AXXNaClInstBytes(state, desc);
          break;
        default:
          NaClConsume0FXXNaClInstBytes(state, desc);
          break;
      }
      break;
    case 0xD8:
    case 0xD9:
    case 0xDA:
    case 0xDB:
    case 0xDC:
    case 0xDD:
    case 0xDE:
    case 0xDF:
      NaClConsumeX87NaClInstBytes(state, desc);
      break;
    default:
      desc->matched_prefix = NoPrefix;
      break;
  }
  DEBUG(NaClLog(LOG_INFO,
                "matched prefix = %s\n",
                NaClInstPrefixName(desc->matched_prefix)));
}

/* Compute the operand and address sizes for the instruction. Then, verify
 * that the opcode (instruction) pattern allows for such sizes. Aborts
 * the pattern match if any problems.
 */
static Bool NaClConsumeAndCheckOperandSize(NaClInstState* state) {
  state->operand_size = NaClExtractOpSize(state);
  DEBUG(NaClLog(LOG_INFO,
                "operand size = %"NACL_PRIu8"\n", state->operand_size));
  if (state->inst->flags &
      (NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSize_v) |
       NACL_IFLAG(OperandSize_o))) {
    NaClIFlags good = 1;
    switch (state->operand_size) {
      case 2:
        good = NaClHasBit(state->inst->flags, NACL_IFLAG(OperandSize_w));
        break;
      case 4:
        good = NaClHasBit(state->inst->flags, NACL_IFLAG(OperandSize_v));
        break;
      case 8:
        good = NaClHasBit(state->inst->flags, NACL_IFLAG(OperandSize_o));
        break;
      default:
        good = 0;
        break;
    }
    if (!good) {
      /* The flags associated with the opcode (instruction) don't
       * allow the computed sizes, abort the  match of the instruction.
       */
      DEBUG(NaClLog(LOG_INFO,
                    "Operand size %"NACL_PRIu8
                    " doesn't match flag requirement!\n",
                    state->operand_size));
      return FALSE;
    }
  }
  return TRUE;
}

static Bool NaClConsumeAndCheckAddressSize(NaClInstState* state) {
  state->address_size = NaClExtractAddressSize(state);
  DEBUG(NaClLog(LOG_INFO,
                "Address size = %"NACL_PRIu8"\n", state->address_size));
  if (state->inst->flags &
      (NACL_IFLAG(AddressSize_w) | NACL_IFLAG(AddressSize_v) |
       NACL_IFLAG(AddressSize_o))) {
    NaClIFlags good = 1;
    switch (state->address_size) {
    case 16:
      good = NaClHasBit(state->inst->flags, NACL_IFLAG(AddressSize_w));
      break;
    case 32:
      good = NaClHasBit(state->inst->flags, NACL_IFLAG(AddressSize_v));
      break;
    case 64:
      good = NaClHasBit(state->inst->flags, NACL_IFLAG(AddressSize_o));
      break;
    default:
      good = 0;
      break;
    }
    if (!good) {
      /* The flags associated with the opcode (instruction) don't
       * allow the computed sizes, abort the  match of the instruction.
       */
      DEBUG(NaClLog(LOG_INFO,
                    "Address size %"NACL_PRIu8
                    " doesn't match flag requirement!\n",
                    state->address_size));
      return FALSE;
    }
  }
  return TRUE;
}

/* Returns true if the instruction requires a ModRm bytes. */
static Bool NaClInstRequiresModRm(NaClInstState* state) {
  return (Bool)
      (NACL_EMPTY_IFLAGS !=
       (state->inst->flags & NACL_IFLAG(OpcodeUsesModRm)));
}

/* Consume the Mod/Rm byte of the instruction, if applicable.
 * Aborts the pattern match if any problems.
 */
static Bool NaClConsumeModRm(NaClInstState* state) {
  /* First check if the opcode (instruction) pattern specifies that
   * a Mod/Rm byte is needed, and that reading it will not walk
   * past the end of the code segment.
   */
  if (NaClInstRequiresModRm(state)) {
    uint8_t byte;
    /* Has modrm byte. */
    if (state->bytes.length >= state->length_limit) {
      DEBUG(NaClLog(LOG_INFO, "Can't read mod/rm, no more bytes!\n"));
      return FALSE;
    }
    byte = NCInstBytesPeekInline(&state->bytes, 0);

    /* Note: Some instructions only allow values where the ModRm mod field
     * is 0x3. Others only allow values where the ModRm mod field isn't 0x3.
     */
    if (modrm_modInline(byte) == 0x3) {
      if (NaClHasBit(state->inst->flags, NACL_IFLAG(ModRmModIsnt0x3))) {
        DEBUG(NaClLog(LOG_INFO, "Can't match, modrm mod field is 0x3\n"));
        return FALSE;
      }
    } else {
      if (NaClHasBit(state->inst->flags, NACL_IFLAG(ModRmModIs0x3))) {
        DEBUG(NaClLog(LOG_INFO, "Can't match, modrm mod field not 0x3\n"));
        return FALSE;
      }
    }
    if ((NaClHasBit(state->inst->flags, NACL_IFLAG(ModRmRegSOperand))) &&
        (modrm_regInline(byte) > 5)) {
      DEBUG(NaClLog(LOG_INFO,
                    "Can't match, modrm reg field doesn't index segment\n"));
      return FALSE;
    }
    state->modrm = NCInstBytesReadInline(&state->bytes);
    state->num_disp_bytes = 0;
    state->first_disp_byte = 0;
    state->sib = 0;
    state->has_sib = FALSE;
    DEBUG(NaClLog(LOG_INFO, "consume modrm = %02"NACL_PRIx8"\n", state->modrm));

    /* Consume the remaining opcode value in the mod/rm byte
     * if applicable.
     */
    if (state->inst->flags & NACL_IFLAG(OpcodeInModRm)) {
      const NaClInst* inst = state->inst;
      if (modrm_opcodeInline(state->modrm) !=
          NaClGetOpcodeInModRm(inst->opcode_ext)) {
        DEBUG(
            NaClLog(LOG_INFO,
                    "Discarding, opcode in mrm byte (%02"NACL_PRIx8") "
                    "does not match\n",
                    modrm_opcodeInline(state->modrm)));
        return FALSE;
      }
      if (state->inst->flags & NACL_IFLAG(OpcodeInModRmRm)) {
        if (modrm_rmInline(state->modrm) !=
            NaClGetOpcodeInModRmRm(inst->opcode_ext)) {
          DEBUG(NaClLog(LOG_INFO,
                        "Discarding, opcode in mrm rm field (%02"NACL_PRIx8") "
                        "does not match\n",
                        modrm_rmInline(state->modrm)));
          return FALSE;
        }
      }
    }
  }
  return TRUE;
}

/* Returns true if the instruction requires a SIB bytes. */
static Bool NaClInstRequiresSib(NaClInstState* state) {
  /* Note: in 64-bit mode, 64-bit addressing is treated the same as 32-bit
   * addressing. Hence, required for all but 16-bit addressing, when
   * the right modrm bytes are specified.
   */
  return (Bool)
      (NaClInstRequiresModRm(state) && (16 != state->address_size) &&
       (modrm_rmInline(state->modrm) ==
        0x04 && modrm_modInline(state->modrm) != 0x3));
}

/* Consume the SIB byte of the instruction, if applicable. Aborts the pattern
 * match if any problems are found.
 */
static Bool NaClConsumeSib(NaClInstState* state) {
  /* First check that the opcode (instruction) pattern specifies that
   * a SIB byte is needed, and that reading it will not walk past
   * the end of the code segment.
   */
  state->sib = 0;
  state->has_sib = NaClInstRequiresSib(state);
  DEBUG(NaClLog(LOG_INFO, "has sib = %d\n", (int) state->has_sib));
  if (state->has_sib) {
    if (state->bytes.length >= state->length_limit) {
      DEBUG(NaClLog(LOG_INFO, "Can't consume sib, no more bytes!\n"));
      return FALSE;
    }
    /* Read the SIB byte and record. */
    state->sib = NCInstBytesReadInline(&state->bytes);
    DEBUG(NaClLog(LOG_INFO, "sib = %02"NACL_PRIx8"\n", state->sib));
    if (sib_base(state->sib) == 0x05 && modrm_modInline(state->modrm) > 2) {
      DEBUG(NaClLog(LOG_INFO,
                    "Sib byte implies modrm.mod field <= 2, match fails\n"));
      return FALSE;
    }
  }
  return TRUE;
}

static int NaClGetNumDispBytes(NaClInstState* state) {
  if (NaClInstRequiresModRm(state)) {
    if (16 == state->address_size) {
      /* Corresponding to table 2-1 of the Intel manual. */
      switch (modrm_modInline(state->modrm)) {
        case 0x0:
          if (modrm_rmInline(state->modrm) == 0x06) {
            return 4;  /* disp16 */
          }
          break;
        case 0x1:
          return 1;    /* disp8 */
        case 0x2:
          return 2;    /* disp16 */
        default:
          break;
      }
    } else {
      /* Note: in 64-bit mode, 64-bit addressing is treated the same as 32-bit
       * addressing. Hence, this section covers the 32-bit addressing.
       */
      switch(modrm_modInline(state->modrm)) {
        case 0x0:
          if (modrm_rmInline(state->modrm) == 0x05) {
            return 4;  /* disp32 */
          } else if (state->has_sib && sib_base(state->sib) == 0x5) {
            return 4;
          }
          break;
        case 0x1:
          return 1;    /* disp8 */
        case 0x2:
          return 4;    /* disp32 */
        default:
          break;
      }
    }
  }
  return 0;
}

/* Consume the needed displacement bytes, if applicable. Abort the
 * pattern match if any problems are found.
 */
static Bool NaClConsumeDispBytes(NaClInstState* state) {
  /* First check if the opcode (instruction) pattern specifies that
   * displacement bytes should be read, and that reading it will not
   * walk past the end of the code segment.
   */
  state->num_disp_bytes = NaClGetNumDispBytes(state);
  DEBUG(NaClLog(LOG_INFO,
                "num disp bytes = %"NACL_PRIu8"\n", state->num_disp_bytes));
  state->first_disp_byte = state->bytes.length;
  if (state->num_disp_bytes > 0) {
    int new_length = state->bytes.length + state->num_disp_bytes;
    if (new_length > state->length_limit) {
      DEBUG(NaClLog(LOG_INFO, "Can't consume disp, no more bytes!\n"));
      return FALSE;
    }
    /* Read the displacement bytes. */
    state->first_disp_byte = state->bytes.length;
    NCInstBytesReadBytesInline(state->num_disp_bytes, &state->bytes);
  }
  return TRUE;
}

/* Returns the number of immediate bytes to parse. */
static int NaClGetNumImmedBytes(NaClInstState* state) {
  if (state->inst->flags & NACL_IFLAG(OpcodeHasImmed)) {
    return state->operand_size;
  }
  if (state->inst->flags & NACL_IFLAG(OpcodeHasImmed_v)) {
    return 4;
  } else if (state->inst->flags & NACL_IFLAG(OpcodeHasImmed_b)) {
    return 1;
  } else if (state->inst->flags & NACL_IFLAG(OpcodeHasImmed_w)) {
    return 2;
  } else if (state->inst->flags & NACL_IFLAG(OpcodeHasImmed_o)) {
    return 8;
  } else if (state->inst->flags & NACL_IFLAG(OpcodeHasImmed_Addr)) {
    return NaClExtractAddressSize(state) / 8;
  } else if (state->inst->flags & NACL_IFLAG(OpcodeHasImmed_z)) {
    if (state->operand_size == 2) {
      return 2;
    } else {
      return 4;
    }
  } else {
    return 0;
  }
}

/* Returns the number of immedidate bytes to parse if a second immediate
 * number appears in the instruction (zero if no second immediate value).
 */
static int NaClGetNumImmed2Bytes(NaClInstState* state) {
  if (state->inst->flags & NACL_IFLAG(OpcodeHasImmed2_b)) {
    return 1;
  } else if (state->inst->flags & NACL_IFLAG(OpcodeHasImmed2_w)) {
    return 2;
  } else if (state->inst->flags & NACL_IFLAG(OpcodeHasImmed2_v)) {
    return 4;
  } else {
    return 0;
  }
}

/* Consume the needed immediate bytes, if applicable. Abort the
 * pattern match if any problems are found.
 */
static Bool NaClConsumeImmediateBytes(NaClInstState* state) {
  /* find out how many immediate bytes are expected. */
  state->num_imm_bytes = NaClGetNumImmedBytes(state);
  DEBUG(NaClLog(LOG_INFO,
                "num immediate bytes = %"NACL_PRIu8"\n", state->num_imm_bytes));
  state->first_imm_byte = 0;
  if (state->num_imm_bytes > 0) {
    int new_length;
    /* Before reading immediate bytes, be sure that we won't walk
     * past the end of the code segment.
     */
    new_length = state->bytes.length + state->num_imm_bytes;
    if (new_length > state->length_limit) {
      DEBUG(NaClLog(LOG_INFO, "Can't consume immediate, no more bytes!\n"));
      return FALSE;
    }
    /* Read the immediate bytes. */
    state->first_imm_byte = state->bytes.length;
    NCInstBytesReadBytesInline(state->num_imm_bytes, &state->bytes);
  }
  /* Before returning, see if second immediate value specified. */
  state->num_imm2_bytes = NaClGetNumImmed2Bytes(state);
  DEBUG(NaClLog(LOG_INFO, "num immediate 2 bytes = %"NACL_PRIu8"\n",
                state->num_imm2_bytes));
  if (state->num_imm2_bytes > 0) {
    int new_length;
    /* Before reading immediate bytes, be sure that we don't walk
     * past the end of the code segment.
     */
    new_length = state->bytes.length + state->num_imm2_bytes;
    if (new_length > state->length_limit) {
      DEBUG(NaClLog(LOG_INFO, "Can't consume 2nd immediate, no more bytes!\n"));
      return FALSE;
    }
    /* Read the immediate bytes. */
    NCInstBytesReadBytesInline(state->num_imm2_bytes, &state->bytes);
  }
  return TRUE;
}

/* Validate that any opcode (instruction) pattern prefix assumptions are
 * met by prefix bits. If not, abort the pattern match.
 */
static Bool NaClValidatePrefixFlags(NaClInstState* state) {
  /* Check lock prefix assumptions. */
  if (state->prefix_mask & kPrefixLOCK) {
    if (state->inst->flags & NACL_IFLAG(OpcodeLockable)) {
      /* Only allow if all destination operands are memory stores. */
      uint32_t i;
      Bool has_lockable_dest = FALSE;
      NaClExpVector* vector = NaClInstStateExpVector(state);
      DEBUG(NaClLog(LOG_INFO, "checking if lock valid on:\n");
            NaClExpVectorPrint(NaClLogGetGio(), vector));
      for (i = 0; i < vector->number_expr_nodes; ++i) {
        NaClExp* node = &vector->node[i];
        DEBUG(NaClLog(LOG_INFO, "  checking node %d\n", i));
        if ((NACL_EMPTY_EFLAGS != (node->flags & NACL_EFLAG(ExprDest))) &&
            (node->kind == ExprMemOffset)) {
          has_lockable_dest = TRUE;
          break;
        }
      }
      if (!has_lockable_dest) {
        DEBUG(NaClLog(LOG_INFO, "Instruction doesn't allow lock prefix "
                      "on non-memory destination"));
        return FALSE;
      }
    } else {
      DEBUG(NaClLog(LOG_INFO, "Instruction doesn't allow lock prefix\n"));
      return FALSE;
    }
  }
  /* Check REX prefix assumptions. */
  if (NACL_TARGET_SUBARCH == 64 &&
      (state->prefix_mask & kPrefixREX)) {
    if (state->inst->flags &
        (NACL_IFLAG(OpcodeUsesRexW) | NACL_IFLAG(OpcodeHasRexR) |
         NACL_IFLAG(OpcodeRex))) {
      if (((state->inst->flags & NACL_IFLAG(OpcodeUsesRexW)) &&
           0 == (state->rexprefix & 0x8)) ||
          ((state->inst->flags & NACL_IFLAG(OpcodeHasRexR)) &&
           0 == (state->rexprefix & 0x4))) {
        DEBUG(NaClLog(LOG_INFO, "can't match REX prefix requirement\n"));
        return FALSE;
      }
    }
  }
  return TRUE;
}

/* Move back to point just after the prefix sequence (defined by
 * inst_length).
 */
static void NaClClearInstState(NaClInstState* state, uint8_t inst_length) {
  if (state->bytes.length != inst_length) {
    NCInstBytesResetInline(&state->bytes);
    NCInstBytesReadBytesInline(inst_length, &state->bytes);
  }
  state->modrm = 0;
  state->has_sib = FALSE;
  state->sib = 0;
  state->num_disp_bytes = 0;
  state->first_disp_byte = 0;
  state->num_imm_bytes = 0;
  state->first_imm_byte = 0;
  state->num_imm2_bytes = 0;
  state->operand_size = 32;
  state->address_size = 32;
  state->nodes.is_defined = FALSE;
  state->nodes.number_expr_nodes = 0;
}

/* Move back to the beginning of the instruction, so that we can reparse. */
static void NaClResetInstState(NaClInstState* state) {
  if (state->bytes.length > 0) {
    NCInstBytesResetInline(&state->bytes);
  }
  state->num_prefix_bytes = 0;
  state->opcode_prefix = 0;
  state->num_opcode_bytes = 0;
  state->rexprefix = 0;
  state->num_rex_prefixes = 0;
  state->modrm = 0;
  state->has_prefix_duplicates = FALSE;
  state->has_ambig_segment_prefixes = FALSE;
  state->has_sib = FALSE;
  state->sib = 0;
  state->num_disp_bytes = 0;
  state->first_disp_byte = 0;
  state->num_imm_bytes = 0;
  state->first_imm_byte = 0;
  state->num_imm2_bytes = 0;
  state->prefix_mask = 0;
  state->inst = NULL;
  state->nodes.is_defined = FALSE;
  state->nodes.number_expr_nodes = 0;
}

/* Get the corresponding instruction for the given offset. */
static const NaClInst* NaClGetOpcodeInst(const NaClDecodeTables *tables,
                                         NaClOpcodeArrayOffset offset) {
  return (NACL_OPCODE_NULL_OFFSET == offset)
      ? NULL
      : &tables->opcodes_table[offset];
}

/* Get the corresponding instruction for the given prefix and opcode. */
static const NaClInst* NaClGetPrefixOpcodeInst(const NaClDecodeTables *tables,
                                               NaClInstPrefix prefix,
                                               uint8_t opcode) {
  const NaClPrefixOpcodeSelector* selector = &tables->opcode_selectors[prefix];
  if ((opcode >= selector->first_opcode) &&
      (opcode <= selector->last_opcode)) {
    return NaClGetOpcodeInst(
        tables,
        tables->opcode_entries[
            selector->table_offset + (opcode - selector->first_opcode)]);
  }
  return NULL;
}

/*
 * Given the opcode prefix descriptor, return the list of candidate opcodes to
 * try and match against the byte stream in the given state. Before returning,
 * this function automatically advances the opcode prefix descriptor to describe
 * the next list to use if the returned list doesn't provide any matches.
 *
 * Parameters:
 *   state - The state of the instruction being decoded.
 *   desc - The description of how the opcode bytes have been matched.
 *      The value passed in is the currrent match, the value at exit is
 *      the value to be used the next time this function is called (to
 *      get the next set of possible instructions).
 *   opcode_length - The length (in bytes) of the opcode for the returned
 *       candidate opcodes.
 */
static const NaClInst* NaClGetNextInstCandidates(
    NaClInstState* state, NaClInstPrefixDescriptor* desc,
    uint8_t* inst_length) {
  const NaClInst* cand_insts;
  if (desc->next_length_adjustment) {
    (*inst_length) += desc->next_length_adjustment;
    desc->opcode_byte = state->bytes.byte[*inst_length - 1];
  }
  cand_insts = NaClGetPrefixOpcodeInst(state->decoder_tables,
                                       desc->matched_prefix,
                                       desc->opcode_byte);
  DEBUG(NaClLog(LOG_INFO, "Lookup candidates using [%s][%x]\n",
                NaClInstPrefixName(desc->matched_prefix), desc->opcode_byte));
  switch (desc->matched_prefix) {
    case Prefix660F:
      desc->matched_prefix = Prefix0F;
      desc->opcode_prefix = 0;
      break;
    case Prefix660F38:
      desc->matched_prefix = Prefix0F38;
      desc->opcode_prefix = 0;
      break;
    case Prefix660F3A:
      desc->matched_prefix = Prefix0F3A;
      desc->opcode_prefix = 0;
      break;
    case PrefixD8:
    case PrefixD9:
    case PrefixDA:
    case PrefixDB:
    case PrefixDC:
    case PrefixDD:
    case PrefixDE:
    case PrefixDF:
      desc->matched_prefix = NoPrefix;
      desc->next_length_adjustment = -1;
      break;
    default:
      /* No more simplier prefices, give up search after current lookup. */
      desc->matched_prefix = NaClInstPrefixEnumSize;
      break;
  }
  return cand_insts;
}

static Bool NaClConsumeHardCodedNop(NaClInstState* state) {
  uint8_t next_byte;
  const NaClInstNode* next;
  uint8_t next_length = 0;
  const NaClInst* matching_inst = NULL;
  uint8_t matching_length = 0;

  next_byte = NCInstByteInline(&state->bytes, next_length);
  next = state->decoder_tables->hard_coded;

  /* Find maximal match in trie. */
  while (NULL != next) {
    if (next_byte == next->matching_byte) {
      DEBUG(NaClLog(LOG_INFO,
                    "NaClConsume opcode char: %"NACL_PRIx8"\n", next_byte));
      next_length++;
      if (NACL_OPCODE_NULL_OFFSET != next->matching_inst) {
        matching_inst = NaClGetOpcodeInst(state->decoder_tables,
                                          next->matching_inst);
        matching_length = next_length;
      }
      if (next_length < state->length_limit) {
        next = next->success;
        if (next != NULL) {
          next_byte = NCInstByteInline(&state->bytes, next_length);
        }
      } else {
        break;
      }
    } else if (next->matching_byte < next_byte) {
      next = next->fail;
    } else {
      break;
    }
  }
  if (NULL == matching_inst) {
    return FALSE;
  } else {
    /* TODO(karl) Make this more general. Currently assumes that no
     * additional processing (other than opcode selection) is needed.
     * This is currently safe only because all instructions modeled
     * using opcode sequences have no (useful) operands, and hence
     * no additional information is needed.
     */
    NaClResetInstState(state);
    state->inst = matching_inst;
    NCInstBytesReadBytesInline(matching_length, &state->bytes);
    DEBUG(NaClLog(LOG_INFO, "matched inst sequence [%d]!\n", matching_length));
    return TRUE;
  }
}

EXTERN_C_END

#endif   /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_STATE_STATICS_C__ */
