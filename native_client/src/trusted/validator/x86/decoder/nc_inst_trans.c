/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Translates the recognized opcode (instruction) in the instruction state
 * into an opcode expression.
 */

#include "native_client/src/trusted/validator/x86/decoder/nc_inst_trans.h"

#include <stdio.h>
#include <assert.h>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_state.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_state_internal.h"
#include "native_client/src/trusted/validator/x86/decoder/ncop_exps.h"
#include "native_client/src/trusted/validator/x86/nacl_regs.h"

#include "native_client/src/trusted/validator/x86/decoder/ncopcode_desc_inl.c"
#include "native_client/src/trusted/validator/x86/x86_insts_inl.c"

#if NACL_TARGET_SUBARCH == 64
# include "native_client/src/trusted/validator/x86/decoder/gen/nc_subregs_64.h"
#else
# include "native_client/src/trusted/validator/x86/decoder/gen/nc_subregs_32.h"
#endif

/* To turn on debugging of instruction decoding, change value of
 * DEBUGGING to 1.
 */
#define DEBUGGING 0

#include "native_client/src/shared/utils/debugging.h"

/* Return the segment register to use, based on prefix specification,
 * or reg_default if no prefix specified.
 */
static NaClOpKind NaClGetSegmentPrefixReg(NaClInstState* state,
                                          NaClOpKind reg_default) {
  /* Note: We need to find the LAST segment prefix byte, since it overrides
   * other segment prefix bytes, if multiple segment prefixes are specified.
   */
  if (32 == NACL_TARGET_SUBARCH) {
    if (state->prefix_mask &
        (kPrefixSEGCS | kPrefixSEGSS | kPrefixSEGFS |
         kPrefixSEGGS | kPrefixSEGES | kPrefixSEGDS)) {
      int i;
      for (i = state->num_prefix_bytes - 1; (i >= 0); --i) {
        switch (state->bytes.byte[i]) {
          case kValueSEGCS:
            return RegCS;
          case kValueSEGSS:
            return RegSS;
          case kValueSEGFS:
            return RegFS;
          case kValueSEGGS:
            return RegGS;
          case kValueSEGES:
            return RegES;
          case kValueSEGDS:
            return RegDS;
          default:
            break;
        }
      }
    }
  } else if (state->prefix_mask &
             /* Only GS and FS matter in 64-bit mode. */
             (kPrefixSEGGS | kPrefixSEGFS)) {
    int i;
    for (i = state->num_prefix_bytes - 1; (i >= 0); --i) {
    /* for (i = 0; i < state->num_prefix_bytes; ++i) { */
      switch (state->bytes.byte[i]) {
        case kValueSEGFS:
          return RegFS;
        case kValueSEGGS:
          return RegGS;
        default:
          break;
      }
    }
  }
  return reg_default;
}

/* Return the segment register to use if DS is the default. */
static INLINE NaClOpKind NaClGetDsSegmentReg(NaClInstState* state) {
  return NaClGetSegmentPrefixReg(state, RegDS);
}

/* Return the segment register to use if ES is the default. */
static INLINE NaClOpKind NaClGetEsSegmentReg(NaClInstState* state) {
  return NaClGetSegmentPrefixReg(state, RegES);
}

/* Append the given expression node onto the given vector of expression
 * nodes. Returns the appended expression node.
 */
static INLINE NaClExp* NaClAppendExp(NaClExpKind kind,
                                     int32_t value,
                                     NaClExpFlags flags,
                                     NaClExpVector* vector) {
  NaClExp* node;
  assert(vector->number_expr_nodes < NACL_MAX_EXPS);
  node = &vector->node[vector->number_expr_nodes++];
  node->kind = kind;
  node->value = value;
  node->flags = flags;
  return node;
}

/* Report the given message and quit because we don't know
 * how to recover.
 */
static void NaClLost(const char* message) {
  NaClLog(LOG_ERROR, "FATAL: %s\n", message);
  exit(1);
}

/* An untranslateable error has been found. report and quit.
 * Use the state to give useful information on where the
 * translator was when the error occurred.
 * Note: returns NULL of type NaClExp* so that callers can
 * make control flow happy, for those cases where the compiler
 * doesn't recognize that this function never returns.
 */
static NaClExp* NaClFatal(const char* message,
                          NaClInstState* state) {
  NaClLog(LOG_ERROR,
          "FATAL: At %"NACL_PRIxNaClPcAddress", unable to translate: %s\n",
          NaClInstStateVpc(state), message);
  exit(1);
  /* NOT REACHED */
  return NULL;
}

/* Returns the segment register encoded in the corresponding
 * mnemonic name of the corresponding instruction.
 */
static NaClOpKind NaClGetMnemonicSegmentRegister(NaClInstState* state) {
  const NaClInst* inst = NaClInstStateInst(state);
  switch (inst->name) {
    case InstLds:
      return RegDS;
    case InstLes:
      return RegES;
    case InstLfs:
      return RegFS;
    case InstLgs:
      return RegGS;
    case InstLss:
      return RegSS;
    default:
      break;
  }
  NaClFatal("Unable to determine segment regsiter from instruction name",
            state);
  /* NOT REACHED */
  return RegUnknown;
}

/* Append the given constant onto the given vector of expression
 * nodes. Returns the appended expression node.
 */
static INLINE NaClExp* NaClAppendConst(uint64_t value, NaClExpFlags flags,
                                       NaClExpVector* vector) {
  uint32_t val1;
  uint32_t val2;
  DEBUG(
      NaClLog(LOG_INFO, "Append constant %"NACL_PRIx64" : ", value);
      NaClPrintExpFlags(NaClLogGetGio(), flags);
      gprintf(NaClLogGetGio(), "\n"));
  NaClSplitExpConstant(value, &val1, &val2);
  if (val2 == 0) {
    return NaClAppendExp(ExprConstant, val1, flags, vector);
  } else {
    NaClExp* root = NaClAppendExp(ExprConstant64, 0, flags, vector);
    NaClAppendExp(ExprConstant, val1, NACL_EFLAG(ExprUnsignedHex), vector);
    NaClAppendExp(ExprConstant, val2, NACL_EFLAG(ExprUnsignedHex), vector);
    return root;
  }
}

/* Define a type corresponding to the arrays NaClRegTable8,
 * NaClRegTable16, NaClRegTable32, and NaClRegTable64.
 */
typedef const NaClOpKind NaClRegTableGroup[NACL_REG_TABLE_SIZE];

NaClOpKind NaClGet64For32BitReg(NaClOpKind reg32) {
#if NACL_TARGET_SUBARCH == 64
  int i;
  for (i = 0; i < NACL_REG_TABLE_SIZE; ++i) {
    if (reg32 == NaClRegTable32[i]) {
      return NaClRegTable64[i];
    }
  }
#endif
  return RegUnknown;
}

NaClOpKind NaClGet32For64BitReg(NaClOpKind reg64) {
  int index = NaClGpReg64Index[reg64];
  return (index == NACL_REGISTER_UNDEFINED)
      ? RegUnknown
      : NaClRegTable32[index];
}

Bool NaClIs64Subreg(NaClInstState* state,
                    NaClOpKind subreg, NaClOpKind reg64) {
  int index = NaClGpSubregIndex[subreg];
  if (index == NACL_REGISTER_UNDEFINED) {
    return FALSE;
  } else {
    int index64 = NaClGpReg64Index[reg64];
    if (index64 == NACL_REGISTER_UNDEFINED) {
      /* This shouldn't happen, so fail! */
      return FALSE;
    } else {
      return index == index64;
    }
  }
}

Bool NaClIs32To64RegPair(NaClOpKind reg32, NaClOpKind reg64) {
  return reg64 == NaClGet64For32BitReg(reg32);
}

/* Define the set of available registers, categorized by size.
 * Note: The order is important, and is based on the indexing values used
 * in the ModRm and SIB bytes (and the REX prefix if appropriate).
 */
static NaClRegTableGroup* const NaClRegTable[] = {
  &NaClRegTable8NoRex,
  &NaClRegTable16,
  &NaClRegTable32,
  &NaClRegTable64,
  &NaClRegTableMmx,
  &NaClRegTableXmm,
  &NaClRegTableC,
  &NaClRegTableD,
};

/* Define possible register categories. */
typedef enum NaClRegKind {
  /* Note: the following all have register tables
   * for the corresponding general purpose registers.
   */
  RegSize8,
  RegSize16,
  RegSize32,
  RegSize64,
  RegMMX,
  RegXMM,
  RegC,
  RegD,
  /* Note: sizes below this point don't define general
   * purpose registers, and hence, don't have a lookup
   * value in the register tables.
   */
  RegSize128,
  RegSizeZ,
  RegUndefined,   /* Always returns RegUnknown. */
} NaClRegKind;

static const char* const g_NaClRegKindName[] = {
  "RegSize8",
  "RegSize16",
  "RegSize32",
  "RegSize64",
  "RegMMX",
  "RegXMM",
  "RegC",
  "RegD",
  "RegSize128",
  "RegSizeZ",
  "RegUndefined"
};

const char* NaClRegKindName(NaClRegKind kind) {
  return g_NaClRegKindName[kind];
}

/* Define ModRm register categories. */
typedef enum NaClModRmRegKind {
  ModRmGeneral,
  ModRmMmx,
  ModRmXmm,
  ModRmCreg,
  ModRmDreg,
  /* Don't allow top level registers in Effective address. */
  ModRmNoTopLevelRegisters
} NaClModRmRegKind;

static const char* const g_NaClModRmRegKindName[] = {
  "ModRmGeneral",
  "ModRmMmx",
  "ModRmXmm",
  "ModRmCreg",
  "ModRmDreg",
  "ModRmNoTopLevelRegisters"
};

/* Given an operand kind, return the size specification associated with
 * the operand kind.
 */
static NaClRegKind NaClGetOpKindRegKind(NaClOpKind kind) {
  switch (kind) {
    case Eb_Operand:
    case Gb_Operand:
    case Ib_Operand:
    case Jb_Operand:
    case Mb_Operand:
    case Ob_Operand:
      return RegSize8;
    case Aw_Operand:
    case Ew_Operand:
    case Gw_Operand:
    case Iw_Operand:
    case Jw_Operand:
    case Mw_Operand:
    case Mpw_Operand:
    case Ow_Operand:
      return RegSize16;
    case Av_Operand:
    case Ev_Operand:
    case Gv_Operand:
    case Iv_Operand:
    case Jv_Operand:
    case Mv_Operand:
    case Mpv_Operand:
    case Ov_Operand:
    case Mmx_Gd_Operand:
      return RegSize32;
    case Ao_Operand:
    case Eo_Operand:
    case Go_Operand:
    case Io_Operand:
    case Mo_Operand:
    case Mpo_Operand:
    case Oo_Operand:
    case Xmm_Eo_Operand:
    case Xmm_Go_Operand:
    case Mmx_E_Operand:
    case Mmx_G_Operand:
      return RegSize64;
    case Edq_Operand:
    case Gdq_Operand:
    case Mdq_Operand:
    case Xmm_E_Operand:
    case Xmm_G_Operand:
      return RegSize128;
    case Seg_G_Operand:
      return RegSizeZ;
    default:
      return RegUndefined;
  }
}

static NaClOpKind NaClLookupReg(NaClInstState* state,
                                NaClRegKind kind, int reg_index) {
  DEBUG(NaClLog(LOG_INFO,
                "Lookup register (rex=%"NACL_PRIx8") %s:%d\n",
                state->rexprefix, NaClRegKindName(kind), reg_index));
  if (32 == NACL_TARGET_SUBARCH && kind == RegSize64) {
    NaClLost("Architecture doesn't define 64 bit registers");
  } else if (RegSize128 <= kind) {
    return RegUnknown;
  }
  if (64 == NACL_TARGET_SUBARCH && kind == RegSize8 && state->rexprefix) {
    return NaClRegTable8Rex[reg_index];
  }
  return (*(NaClRegTable[kind]))[reg_index];
}

/* Returns the (NaClExpFlag) size of the given register. */
static NaClExpFlags NaClGetRegSize(NaClOpKind register_name) {
  switch (register_name) {
  case RegAL:
  case RegBL:
  case RegCL:
  case RegDL:
  case RegAH:
  case RegBH:
  case RegCH:
  case RegDH:
  case RegDIL:
  case RegSIL:
  case RegBPL:
  case RegSPL:
  case RegR8B:
  case RegR9B:
  case RegR10B:
  case RegR11B:
  case RegR12B:
  case RegR13B:
  case RegR14B:
  case RegR15B:
    return NACL_EFLAG(ExprSize8);
  case RegAX:
  case RegBX:
  case RegCX:
  case RegDX:
  case RegSI:
  case RegDI:
  case RegBP:
  case RegSP:
  case RegR8W:
  case RegR9W:
  case RegR10W:
  case RegR11W:
  case RegR12W:
  case RegR13W:
  case RegR14W:
  case RegR15W:
    return NACL_EFLAG(ExprSize16);
  case RegEAX:
  case RegEBX:
  case RegECX:
  case RegEDX:
  case RegESI:
  case RegEDI:
  case RegEBP:
  case RegESP:
  case RegR8D:
  case RegR9D:
  case RegR10D:
  case RegR11D:
  case RegR12D:
  case RegR13D:
  case RegR14D:
  case RegR15D:
    return NACL_EFLAG(ExprSize32);
  case RegCS:
  case RegDS:
  case RegSS:
  case RegES:
  case RegFS:
  case RegGS:
    return NACL_EFLAG(ExprSize16);
  case RegEIP:
    return NACL_EFLAG(ExprSize32);
  case RegRIP:
    return NACL_EFLAG(ExprSize64);
  case RegRAX:
  case RegRBX:
  case RegRCX:
  case RegRDX:
  case RegRSI:
  case RegRDI:
  case RegRBP:
  case RegRSP:
  case RegR8:
  case RegR9:
  case RegR10:
  case RegR11:
  case RegR12:
  case RegR13:
  case RegR14:
  case RegR15:
    return NACL_EFLAG(ExprSize64);
  default:
    return 0;
  }
}

/* Appends the given kind of register onto the vector of expression nodes.
 * Returns the appended register.
 */
static INLINE NaClExp* NaClAppendReg(NaClOpKind r, NaClExpVector* vector) {
  NaClExp* node;
  DEBUG(NaClLog(LOG_INFO, "append register %s\n", NaClOpKindName(r)));
  node = NaClAppendExp(ExprRegister, r, NaClGetRegSize(r), vector);
  DEBUG(NaClExpVectorPrint(NaClLogGetGio(), vector));
  return node;
}

/* Given the given register kind, and the corresponding index, append
 * the appropriate register onto the vector of expression nodes.
 * Returns the appended register
 */
static INLINE NaClExp* NaClAppendRegKind(NaClInstState* state,
                                         NaClRegKind kind, int reg_index) {
  DEBUG(NaClLog(LOG_INFO, "NaClAppendRegKind(%d, %d) = %s\n",
                (int) kind, reg_index, NaClRegKindName(kind)));
  return NaClAppendReg(NaClLookupReg(state, kind, reg_index), &state->nodes);
}

/* Given an operand of the corresponding opcode instruction of the
 * given state, return what kind of register should be used, based
 * on the operand size.
 */
static NaClRegKind NaClExtractOpRegKind(NaClInstState* state,
                                        const NaClOp* operand) {
  NaClRegKind reg_kind = NaClGetOpKindRegKind(operand->kind);
  switch (reg_kind) {
    case RegSize8:
    case RegSize16:
    case RegSize32:
    case RegSize64:
      return reg_kind;
    case RegSizeZ:
      if (state->operand_size == 2) {
        return RegSize16;
      } else {
        return RegSize32;
      }
    default:
      /* Size not explicitly defined, pick up from operand size. */
      if (state->inst->flags & NACL_IFLAG(OperandSize_b)) {
        return RegSize8;
      } else if (state->operand_size == 1) {
        return RegSize8;
      } else if (state->operand_size == 4) {
        return RegSize32;
      } else if (state->operand_size == 2) {
        return RegSize16;
      } else if (state->operand_size == 8) {
        return RegSize64;
      } else {
        return RegSize32;
      }
  }
}

/* Given an address of the corresponding opcode instruction of the
 * given state, return what kind of register should be used.
 */
static INLINE NaClRegKind NaClExtractAddressRegKind(NaClInstState* state) {
  if (state->address_size == 16) {
    return RegSize16;
  } else if (state->address_size == 64) {
    return RegSize64;
  } else {
    return RegSize32;
  }
}

/* Given we want to translate an operand (of the form G_Operand),
 * for the given register index, generate the corresponding register
 * expression, and append it to the vector of expression nodes.
 * Returns the appended register.
 */
static NaClExp* NaClAppendOperandReg(
    NaClInstState* state,
    const NaClOp* operand,
    int reg_index,
    NaClModRmRegKind modrm_reg_kind) {
  NaClRegKind reg_kind = RegSize32;
  DEBUG(NaClLog(LOG_INFO, "modrm_reg_kind = %s\n",
                g_NaClModRmRegKindName[modrm_reg_kind]));
  switch (modrm_reg_kind) {
    default:
    case ModRmGeneral:
      reg_kind = NaClExtractOpRegKind(state, operand);
      break;
    case ModRmMmx:
      reg_kind = RegMMX;
      break;
    case ModRmXmm:
      reg_kind = RegXMM;
      break;
    case ModRmCreg:
      reg_kind = RegC;
      break;
    case ModRmDreg:
      reg_kind = RegD;
      break;
    case ModRmNoTopLevelRegisters:
      reg_kind = RegUndefined;
  }
  DEBUG(NaClLog(LOG_INFO, "Translate register %d, %s\n",
                reg_index, g_NaClRegKindName[reg_kind]));
  return NaClAppendRegKind(state, reg_kind, reg_index);
}

static NaClExpFlags NaClGetAddressExprSizeFlagsForState(NaClInstState* state);

static NaClExp* NaClAppendSegmentAddress(NaClInstState* state) {
  NaClExpFlags flags = NaClGetAddressExprSizeFlagsForState(state);
  return NaClAppendExp(ExprSegmentAddress, 0, flags, &state->nodes);
}

/* Same as NaClAppendOperandReg, except that a segment register is combined with
 * the indexed register to define a segment address.
 */
static NaClExp* NaClAppendSegmentOpReg(
    NaClInstState* state,
    const NaClOp* operand,
    NaClOpKind seg_register,
    int reg_index,
    NaClModRmRegKind modrm_reg_kind) {
  NaClExp* results = NaClAppendSegmentAddress(state);
  NaClAppendReg(seg_register, &state->nodes);
  NaClAppendOperandReg(state, operand, reg_index, modrm_reg_kind);
  return results;
}

/* Returns the corresponding segment register for the given index (0..7) */
static NaClExp* NaClAppendModRmSegmentReg(NaClInstState* state) {
  static NaClOpKind seg[8] = {
    RegES,
    RegCS,
    RegSS,
    RegDS,
    RegFS,
    RegGS,
    /* These should not happen. */
    RegUnknown,
    RegUnknown
  };
  return NaClAppendReg(seg[modrm_regInline(state->modrm)], &state->nodes);
}

/* For the given instruction state, and the corresponding 3-bit specification
 * of a register, update it to a 4-bit specification, based on the REX.R bit.
 */
static INLINE int NaClGetRexRReg(NaClInstState* state, int reg) {
  DEBUG(NaClLog(LOG_INFO, "Get GenRexRRegister %d\n", reg));
  if (NACL_TARGET_SUBARCH == 64 && (state->rexprefix & 0x4)) {
    reg += 8;
  }
  return reg;
}

/* For the given instruction state, and the corresponding 3-bit specification
 * of a register, update it to a 4-bit specification, based on the REX.X bit.
 */
static INLINE int NaClGetRexXReg(NaClInstState* state, int reg) {
  DEBUG(NaClLog(LOG_INFO, "Get GenRexXRegister\n"));
  if (NACL_TARGET_SUBARCH == 64 && (state->rexprefix & 0x2)) {
    reg += 8;
  }
  return reg;
}

/* For the given instruction state, and the corresponding 3-bit specification
 * of a register, update it to a 4-bit specification, based on the REX.B bit.
 */
static INLINE int NaClGetRexBReg(NaClInstState* state, int reg) {
  DEBUG(NaClLog(LOG_INFO, "Get GenRexBRegister\n"));
  if (NACL_TARGET_SUBARCH == 64 && (state->rexprefix & 0x1)) {
    DEBUG(NaClLog(LOG_INFO, "rexprefix == %02x\n", state->rexprefix));
    reg += 8;
  }
  return reg;
}

/* Return the general purpose register associated with the modrm.reg
 * field.
 */
static INLINE int NaClGetGenRegRegister(NaClInstState* state) {
  DEBUG(NaClLog(LOG_INFO, "Get GenRegRegister\n"));
  return NaClGetRexRReg(state, modrm_regInline(state->modrm));
}

/* Return the general purpose register associated with the modrm.rm
 * field.
 */
static INLINE int NaClGetGenRmRegister(NaClInstState* state) {
  DEBUG(NaClLog(LOG_INFO, "Get GenRmRegister\n"));
  return NaClGetRexBReg(state, modrm_rmInline(state->modrm));
}

/* Get the register index from the difference of the opcode, and
 * its opcode base.
 */
static NaClExp* NaClAppendOpcodeBaseReg(
    NaClInstState* state, const NaClOp* operand) {
  int reg_index;
  reg_index = NaClGetOpcodePlusR(state->inst->opcode_ext);
  assert(reg_index >= 0 && reg_index < 8);
  DEBUG(NaClLog(LOG_INFO, "Translate opcode base register %d\n", reg_index));
  return NaClAppendRegKind(state, NaClExtractOpRegKind(state, operand),
                            NaClGetRexBReg(state, reg_index));
}

/* Get the ST register defined from the difference of the opcode, and
 * its opcode base.
 */
static NaClExp* NaClAppendStOpcodeBaseReg(NaClInstState* state) {
  int reg_index;
  reg_index = NaClGetOpcodePlusR(state->inst->opcode_ext);
  assert(reg_index >= 0 && reg_index < 8);
  DEBUG(NaClLog(LOG_INFO, "Translate opcode base register %d\n", reg_index));
  return NaClAppendReg(RegST0 + reg_index, &state->nodes);
}

/* Model the extraction of a displacement value and the associated flags. */
typedef struct NaClDisplacement {
  uint64_t value;
  NaClExpFlags flags;
} NaClDisplacement;

static INLINE void NaClInitializeDisplacement(
    uint64_t value, NaClExpFlags flags,
    NaClDisplacement* displacement) {
  displacement->value = value;
  displacement->flags = flags;
}

/* Extract the binary value from the specified bytes of the instruction. */
uint64_t NaClExtractUnsignedBinaryValue(NaClInstState* state,
                                        int start_byte, int num_bytes) {
  int i;
  uint64_t value = 0;
  for (i = 0; i < num_bytes; ++i) {
    uint8_t byte = state->bytes.byte[start_byte + i];
    value += (((uint64_t) byte) << (i * 8));
  }
  return value;
}

int64_t NaClExtractSignedBinaryValue(NaClInstState* state,
                                     int start_byte, int num_bytes) {
  int i;
  int64_t value = 0;
  for (i = 0; i < num_bytes; ++i) {
    uint8_t byte = state->bytes.byte[start_byte + i];
    value |= (((uint64_t) byte) << (i * 8));
  }
  return value;
}

/* Given the number of bytes for a literal constant, return the corresponding
 * expr node flags that represent the value of the parsed bytes.
 */
static NaClExpFlags NaClGetExprSizeFlagForBytes(uint8_t num_bytes) {
  switch (num_bytes) {
    case 1:
      return NACL_EFLAG(ExprSize8);
      break;
    case 2:
      return NACL_EFLAG(ExprSize16);
      break;
    case 4:
      return NACL_EFLAG(ExprSize32);
    case 8:
      return NACL_EFLAG(ExprSize64);
    default:
      return 0;
  }
}

/* Return the expr flag for the address size associated with the state. */
static NaClExpFlags NaClGetAddressExprSizeFlagsForState(NaClInstState* state) {
  uint8_t size = NaClInstStateAddressSize(state);
  return NaClGetExprSizeFlagForBytes(size / 8);
}

/* Given the corresponding instruction state, return the
 * corresponding displacement value, and any expression node
 * flags that should be associated with the displacement value.
 */
static void NaClExtractDisplacement(NaClInstState* state,
                                    NaClDisplacement* displacement,
                                    NaClExpFlags flags) {
  DEBUG(NaClLog(LOG_INFO, "-> Extract displacement, flags = ");
        NaClPrintExpFlags(NaClLogGetGio(), flags);
        gprintf(NaClLogGetGio(), "\n"));
  /* First compute the displacement value. */
  displacement->value = NaClExtractUnsignedBinaryValue(state,
                                                       state->first_disp_byte,
                                                       state->num_disp_bytes);

  /* Now compute any appropriate flags to be associated with the value. */
  displacement->flags = flags |
      NaClGetExprSizeFlagForBytes(state->num_disp_bytes);
  DEBUG(NaClLog(LOG_INFO,
                "<- value = %"NACL_PRIx64", flags = ", displacement->value);
        NaClPrintExpFlags(NaClLogGetGio(), displacement->flags);
        gprintf(NaClLogGetGio(), "\n"));
}

/* Append the displacement value of the given instruction state
 * onto the vector of expression nodes. Returns the appended displacement
 * value.
 */
static NaClExp* NaClAppendDisplacement(NaClInstState* state) {
  NaClDisplacement displacement;
  DEBUG(NaClLog(LOG_INFO, "append displacement\n"));
  NaClExtractDisplacement(state, &displacement, NACL_EFLAG(ExprSignedHex));
  return NaClAppendConst(displacement.value, displacement.flags, &state->nodes);
}

/* Get the binary value denoted by the immediate bytes of the state. */
static uint64_t NaClExtractUnsignedImmediate(NaClInstState* state) {
  return NaClExtractUnsignedBinaryValue(state,
                                        state->first_imm_byte,
                                        state->num_imm_bytes);
}

/* Get the binary value denoted by the 2nd immediate bytes of the state. */
static uint64_t NaClExtractUnsignedImmediate2(NaClInstState* state) {
  return NaClExtractUnsignedBinaryValue(
      state,
      state->first_imm_byte + state->num_imm_bytes,
      state->num_imm2_bytes);
}

/* Get the binary value denoted by the immediate bytes of the state. */
static int64_t NaClExtractSignedImmediate(NaClInstState* state) {
  return NaClExtractSignedBinaryValue(state,
                                      state->first_imm_byte,
                                      state->num_imm_bytes);
}

/* Append the immediate value of the given instruction state onto
 * The vector of expression nodes. Returns the appended immediate value.
 */
static NaClExp* NaClAppendImmed(NaClInstState* state) {
  NaClExpFlags flags;

  /* First compute the immediate value. */
  uint64_t value;
  DEBUG(NaClLog(LOG_INFO, "append immediate\n"));
  value = NaClExtractUnsignedImmediate(state);

  /* Now compute any appropriate flags to be associated with the immediate
   * value.
   */
  flags = NACL_EFLAG(ExprUnsignedHex) |
      NaClGetExprSizeFlagForBytes(state->num_imm_bytes);

  /* Append the generated immediate value onto the vector. */
  return NaClAppendConst(value, flags,  &state->nodes);
}

/* Append the second immediate value of the given instruction state onto
 * the vector of expression nodes. Returns the appended immediate value.
 */
static NaClExp* NaClAppendImmed2(NaClInstState* state) {
  NaClExpFlags flags;

  /* First compute the immedaite value. */
  uint64_t value;
  DEBUG(NaClLog(LOG_INFO, "append 2nd immediate\n"));

  value = NaClExtractUnsignedImmediate2(state);

  /* Now compute any appropriate flags to be associated with the immediate
   * value.
   */
  flags =
      NACL_EFLAG(ExprUnsignedHex) |
      NaClGetExprSizeFlagForBytes(state->num_imm2_bytes);

  /* Append the generated immediate value onto the vector. */
  return NaClAppendConst(value, flags,  &state->nodes);
}

/* Append an ExprMemOffset node for the given state, and return
 * the appended ndoe.
 */
static NaClExp* NaClAppendMemOffsetNode(NaClInstState* state) {
  NaClExpFlags flags = NaClGetAddressExprSizeFlagsForState(state);
  NaClExp* root = NaClAppendExp(ExprMemOffset, 0, flags, &state->nodes);
  DEBUG(NaClLog(LOG_INFO, "Build memoffset, flags = ");
        NaClPrintExpFlags(NaClLogGetGio(), flags);
        gprintf(NaClLogGetGio(), "\n"));
  return root;
}

/* Returns the segment register prefix node, or NULL if no such node is
 * added.
 */
static NaClExp* NaClAppendSegmentAddressNode(NaClInstState* state,
                                             NaClOpKind reg_default) {
  NaClExp* root = NULL;
  NaClOpKind seg_reg = NaClGetSegmentPrefixReg(state, reg_default);
  if (seg_reg != RegUnknown) {
    NaClExp* n;
    root = NaClAppendSegmentAddress(state);
    n = NaClAppendReg(seg_reg, &state->nodes);
    n->flags |= NACL_EFLAG(ExprUsed);
  }
  return root;
}

/* Append the immediate value of the given instruction as the displacement
 * of a memory offset.
 */
static NaClExp* NaClAppendMemoryOffsetImmed(NaClInstState* state) {
  NaClExpFlags flags;
  uint64_t value;
  NaClExp* root;
  DEBUG(NaClLog(LOG_INFO, "append memory offset immediate\n"));
  root = NaClAppendSegmentAddressNode(state, RegUnknown);
  if (root == NULL) {
    root = NaClAppendMemOffsetNode(state);
  } else {
    NaClAppendMemOffsetNode(state);
  }
  NaClAppendReg(RegUnknown, &state->nodes);
  NaClAppendReg(RegUnknown, &state->nodes);
  NaClAppendConst(1, NACL_EFLAG(ExprSize8), &state->nodes);
  value = NaClExtractUnsignedImmediate(state);
  DEBUG(NaClLog(LOG_INFO, "value = 0x%016"NACL_PRIx64"\n", value));
  flags = NACL_EFLAG(ExprUnsignedHex) |
      NaClGetExprSizeFlagForBytes(state->num_imm_bytes);
  NaClAppendConst(value, flags, &state->nodes);
  return root;
}

/* Compute the (relative) immediate value defined by the difference
 * between the PC and the immedaite value of the instruction.
 */
static NaClExp* NaClAppendRelativeImmed(NaClInstState* state) {
  NaClPcNumber next_pc = (NaClPcNumber) state->vpc + state->bytes.length;
  NaClPcNumber val = (NaClPcNumber) NaClExtractSignedImmediate(state);

  DEBUG(NaClLog(LOG_INFO, "append relative immediate\n"));

  /* Sign extend value */
  switch (state->num_imm_bytes) {
    case 1:
      val = next_pc + (int8_t) val;
      break;
    case 2:
      val = next_pc + (int16_t) val;
      break;
    case 4:
      val = next_pc + (int32_t) val;
      break;
    default:
      assert(0);
      break;
  }

  return NaClAppendConst(val,
                         NACL_EFLAG(ExprUnsignedHex) |
                         NACL_EFLAG(ExprJumpTarget),
                         &state->nodes);
}

/* Append a memory offset for the given memory offset defined by
 * the formula "base + index*scale + displacement". If no index
 * is used, its value should be RegUnknown. Argument displacement_flags
 * are flags that should be associated with the generated displacement
 * value
 */
static NaClExp* NaClAppendMemoryOffset(NaClInstState* state,
                                       NaClOpKind base,
                                       NaClOpKind index,
                                       uint8_t scale,
                                       NaClDisplacement* displacement) {
  NaClExp* root = NULL;
  NaClOpKind seg_reg_default;
  NaClExp* n;

  DEBUG(NaClLog(LOG_INFO,
                "memory offset(%s + %s * %d +  %"NACL_PRId64
                " : %"NACL_PRIx32")\n",
                NaClOpKindName(base),
                NaClOpKindName(index),
                scale,
                displacement->value,
                displacement->flags));

  if (32 == NACL_TARGET_SUBARCH) {
    seg_reg_default = ((base == RegBP || base == RegEBP)
                       ? RegSS : NaClGetDsSegmentReg(state));
  } else {
    seg_reg_default = RegUnknown;
  }
  root = NaClAppendSegmentAddressNode(state, seg_reg_default);
  if (NULL == root) {
    root = NaClAppendMemOffsetNode(state);
  } else {
    NaClAppendMemOffsetNode(state);
  }
  n = NaClAppendReg(base, &state->nodes);
  if (base != RegUnknown) {
    n->flags |= NACL_EFLAG(ExprUsed);
  }
  n = NaClAppendReg(index, &state->nodes);
  if (index == RegUnknown) {
    /* Scale not applicable, check that value is 1. */
    assert(scale == 1);
  } else {
    n->flags |= NACL_EFLAG(ExprUsed);
  }
  NaClAppendConst(scale, NACL_EFLAG(ExprSize8), &state->nodes);
  NaClAppendConst(displacement->value, displacement->flags, &state->nodes);
  DEBUG(NaClLog(LOG_INFO, "finished appending memory offset:\n"));
  DEBUG(NaClExpVectorPrint(NaClLogGetGio(), &state->nodes));
  return root;
}

/* Extract the base register from the SIB byte. */
static NaClOpKind NaClGetSibBase(NaClInstState* state) {
  int base = sib_base(state->sib);
  NaClOpKind base_reg = RegUnknown;
  if (0x5 == base) {
    switch (modrm_modInline(state->modrm)) {
      case 0:
        break;
      case 1:
      case 2:
        if (NACL_TARGET_SUBARCH == 64) {
          if (state->rexprefix & 0x1) {
            base_reg = RegR13;
          } else {
            base_reg = RegRBP;
          }
        } else {
          base_reg = RegEBP;
        }
        break;
      default:
        NaClFatal("SIB value", state);
    }
  } else {
    NaClRegKind kind = NaClExtractAddressRegKind(state);
    base_reg = NaClLookupReg(state, kind, NaClGetRexBReg(state, base));
  }
  return base_reg;
}

/* Define the possible scaling factors that can be defined in the
 * SIB byte of the parsed instruction.
 */
static uint8_t nacl_sib_scale[4] = { 1, 2, 4, 8 };

/* Extract out the expression defined by the SIB byte of the instruction
 * in the given instruction state, and append it to the vector of
 * expression nodes. Return the corresponding expression node that
 * is the root of the appended expression.
 */
static NaClExp* NaClAppendSib(NaClInstState* state) {
  int index = sib_index(state->sib);
  int scale = 1;
  NaClRegKind kind = NaClExtractAddressRegKind(state);
  NaClOpKind base_reg;
  NaClOpKind index_reg = RegUnknown;
  NaClDisplacement displacement;
  DEBUG(NaClLog(LOG_INFO, "append sib: %02x\n", state->sib));
  NaClInitializeDisplacement(0, 0, &displacement);
  base_reg = NaClGetSibBase(state);
  if (0x4 != index || NACL_TARGET_SUBARCH != 64 || (state->rexprefix & 0x2)) {
    index_reg = NaClLookupReg(state, kind, NaClGetRexXReg(state, index));
    scale = nacl_sib_scale[sib_ss(state->sib)];
  }
  if (state->num_disp_bytes > 0) {
    NaClExtractDisplacement(state, &displacement,
                        NACL_EFLAG(ExprSignedHex));
  } else {
    displacement.flags = NACL_EFLAG(ExprSize8);
  }
  return NaClAppendMemoryOffset(state, base_reg, index_reg,
                                scale, &displacement);
}

static void NaClAppendEDI(NaClInstState* state) {
  switch (state->address_size) {
    case 16:
      NaClAppendReg(RegDI, &state->nodes);
      break;
    case 32:
      NaClAppendReg(RegEDI, &state->nodes);
      break;
    case 64:
      NaClAppendReg(RegRDI, &state->nodes);
      break;
    default:
      NaClFatal("Address size for ES:EDI not correctly defined", state);
      break;
  }
}

static void NaClAppendEBX(NaClInstState* state) {
  switch (state->address_size) {
    case 16:
      NaClAppendReg(RegBX, &state->nodes);
      break;
    case 32:
      NaClAppendReg(RegEBX, &state->nodes);
      break;
    case 64:
      NaClAppendReg(RegRDX, &state->nodes);
      break;
    default:
      NaClFatal("Address size for DS:EDX not correctly defined", state);
      break;
  }
}

static NaClExp* NaClAppendDS_EDI(NaClInstState* state) {
  NaClExp* results = NaClAppendSegmentAddress(state);
  results->flags |= NACL_EFLAG(ExprDSrDICase);
  NaClAppendReg(NaClGetDsSegmentReg(state),  &state->nodes);
  NaClAppendEDI(state);
  return results;
}

static NaClExp* NaClAppendDS_EBX(NaClInstState* state) {
  NaClExp* results = NaClAppendSegmentAddress(state);
  results->flags |= NACL_EFLAG(ExprDSrDICase);
  NaClAppendReg(NaClGetDsSegmentReg(state),  &state->nodes);
  NaClAppendEBX(state);
  return results;
}

static NaClExp* NaClAppendES_EDI(NaClInstState* state) {
  NaClExp* results = NaClAppendSegmentAddress(state);
  results->flags |= NACL_EFLAG(ExprESrDICase);
  NaClAppendReg(NaClGetEsSegmentReg(state),  &state->nodes);
  NaClAppendEDI(state);
  return results;
}

/* Get the Effective address in the mod/rm byte, if the modrm.mod field
 * is 00, and append it to the vector of expression nodes. Operand is
 * the corresponding operand of the opcode associated with the instruction
 * of the given state that corresponds to the effective address. Returns
 * the root of the appended effective address.
 */
static NaClExp* NaClAppendMod00EffectiveAddress(
    NaClInstState* state, const NaClOp* operand) {
  DEBUG(NaClLog(LOG_INFO, "Translate modrm(%02x).mod == 00\n", state->modrm));
  switch (modrm_rmInline(state->modrm)) {
    case 4:
      return NaClAppendSib(state);
    case 5:
      if (NACL_TARGET_SUBARCH == 64) {
        NaClDisplacement displacement;
        NaClExtractDisplacement(state, &displacement,
                                NACL_EFLAG(ExprSignedHex));
        return NaClAppendMemoryOffset(state,
                                      RegRIP,
                                      RegUnknown,
                                      1,
                                      &displacement);
      } else {
        return NaClAppendDisplacement(state);
      }
    default: {
      NaClDisplacement displacement;
      NaClInitializeDisplacement(0, NACL_EFLAG(ExprSize8), &displacement);
      return NaClAppendMemoryOffset(state,
                                    NaClLookupReg(
                                        state,
                                        NaClExtractAddressRegKind(state),
                                        NaClGetGenRmRegister(state)),
                                    RegUnknown,
                                    1,
                                    &displacement);
    }
  }
  /* NOT REACHED */
  return NULL;
}

/* Get the Effective address in the mod/rm byte, if the modrm.mod field
 * is 01, and append it to the vector of expression nodes. Operand is
 * the corresponding operand of the opcode associated with the instruction
 * of the given state that corresponds to the effective address. Returns
 * the root of the appended effective address.
 */
static NaClExp* NaClAppendMod01EffectiveAddress(
    NaClInstState* state, const NaClOp* operand) {
  DEBUG(NaClLog(LOG_INFO, "Translate modrm(%02x).mod == 01\n", state->modrm));
  if (4 == modrm_rmInline(state->modrm)) {
    return NaClAppendSib(state);
  } else {
    NaClDisplacement displacement;
    NaClExtractDisplacement(state, &displacement, NACL_EFLAG(ExprSignedHex));
    return NaClAppendMemoryOffset(state,
                                  NaClLookupReg(
                                      state,
                                      NaClExtractAddressRegKind(state),
                                      NaClGetGenRmRegister(state)),
                                  RegUnknown,
                                  1,
                                  &displacement);
  }
}

/* Get the Effective address in the mod/rm byte, if the modrm.mod field
 * is 10, and append it to the vector of expression nodes. Operand is
 * the corresponding operand of the opcode associated with the instruction
 * of the given state that corresponds to the effective address. Returns
 * the root of the appended effective address.
 */
static NaClExp* NaClAppendMod10EffectiveAddress(
    NaClInstState* state, const NaClOp* operand) {
  DEBUG(NaClLog(LOG_INFO, "Translate modrm(%02x).mod == 10\n", state->modrm));
  if (4 == modrm_rmInline(state->modrm)) {
    return NaClAppendSib(state);
  } else {
    NaClDisplacement displacement;
    NaClOpKind base =
        NaClLookupReg(state,
                      NaClExtractAddressRegKind(state),
                      NaClGetGenRmRegister(state));
    NaClExtractDisplacement(state, &displacement, NACL_EFLAG(ExprSignedHex));
    return NaClAppendMemoryOffset(state, base, RegUnknown, 1, &displacement);
  }
}

/* Get the Effective address in the mod/rm byte, if the modrm.mod field
 * is 11, and append it to the vector of expression nodes. Operand is
 * the corresponding operand of the opcode associated with the instruction
 * of the given state that corresponds to the effective address. Returns
 * the root of the appended effective address.
 */
static NaClExp* NaClAppendMod11EffectiveAddress(
    NaClInstState* state, const NaClOp* operand,
    NaClModRmRegKind modrm_reg_kind) {
  DEBUG(NaClLog(LOG_INFO, "Translate modrm(%02x).mod == 11, %s\n",
                state->modrm, g_NaClModRmRegKindName[modrm_reg_kind]));
  return NaClAppendOperandReg(state,
                              operand,
                              NaClGetGenRmRegister(state), modrm_reg_kind);
}

static NaClExp* NaClAppendBasedOnSize(NaClOpKind reg_2b,
                                      NaClOpKind reg_4b,
                                      NaClOpKind reg_8b,
                                      NaClInstState* state) {
  switch (state->operand_size) {
    case 2:
      return NaClAppendReg(reg_2b, &state->nodes);
    case 4:
      return NaClAppendReg(reg_4b, &state->nodes);
    case 8:
      return NaClAppendReg(reg_8b, &state->nodes);
    default:
      return NaClFatal("can't translate register group: operand size not valid",
                       state);
  }
}

static NaClExp* NaClAppendBasedOnAddressSize(NaClOpKind reg_2b,
                                             NaClOpKind reg_4b,
                                             NaClOpKind reg_8b,
                                             NaClInstState* state) {
  switch (state->address_size) {
    case 16:
      return NaClAppendReg(reg_2b, &state->nodes);
    case 32:
      return NaClAppendReg(reg_4b, &state->nodes);
    case 64:
      return NaClAppendReg(reg_8b, &state->nodes);
    default:
      return NaClFatal("can't translate register group: address size not valid",
                       state);
  }
}

/* Compute the effect address using the Mod/Rm and SIB bytes. */
static NaClExp* NaClAppendEffectiveAddress(
    NaClInstState* state, const NaClOp* operand,
    NaClModRmRegKind modrm_reg_kind) {
  switch(modrm_modInline(state->modrm)) {
    case 0:
      return NaClAppendMod00EffectiveAddress(state, operand);
    case 1:
      return NaClAppendMod01EffectiveAddress(state, operand);
    case 2:
      return NaClAppendMod10EffectiveAddress(state, operand);
    case 3:
      return NaClAppendMod11EffectiveAddress(state, operand, modrm_reg_kind);
    default:
      break;
  }
  return NaClFatal("Operand", state);
}

/* Given the corresponding operand of the opcode associated with the
 * instruction of the given state, append the corresponding expression
 * nodes that it corresponds to. Returns the root of the corresponding
 * appended expression tree.
 */
static NaClExp* NaClAppendOperand(NaClInstState* state,
                                  const NaClOp* operand) {
  DEBUG(NaClLog(LOG_INFO,
                "append operand %s\n", NaClOpKindName(operand->kind)));
  switch (operand->kind) {
    case E_Operand:
    case Eb_Operand:
    case Ew_Operand:
    case Ev_Operand:
    case Eo_Operand:
    case Edq_Operand:
      /* TODO(karl) Should we add limitations that simple registers
       * not allowed in M_Operand cases?
       */
    case M_Operand:
    case Mb_Operand:
    case Mw_Operand:
    case Mv_Operand:
    case Mo_Operand:
    case Mdq_Operand: {
        NaClExp* address =
            NaClAppendEffectiveAddress(state, operand, ModRmGeneral);
        /* Near operands are jump addresses. Mark them as such. */
        if (operand->flags & NACL_OPFLAG(OperandNear)) {
          address->flags |= NACL_EFLAG(ExprJumpTarget);
        }
        return address;
      }
      break;
    case G_Operand:
    case Gb_Operand:
    case Gw_Operand:
    case Gv_Operand:
    case Go_Operand:
    case Gdq_Operand:
      return NaClAppendOperandReg(state, operand, NaClGetGenRegRegister(state),
                                  ModRmGeneral);
    case Seg_G_Operand:
      return NaClAppendSegmentOpReg(
          state, operand, NaClGetMnemonicSegmentRegister(state),
          NaClGetGenRegRegister(state), ModRmGeneral);
    case G_OpcodeBase:
      return NaClAppendOpcodeBaseReg(state, operand);
    case I_Operand:
    case Ib_Operand:
    case Iw_Operand:
    case Iv_Operand:
    case Io_Operand:
      return NaClAppendImmed(state);
    case I2_Operand:
      return NaClAppendImmed2(state);
    case J_Operand:
    case Jb_Operand:
    case Jw_Operand:
    case Jv_Operand:
      /* TODO(karl) use operand flags OperandNear and OperandRelative to decide
       * how to process the J operand (see Intel manual for call statement).
       */
      return NaClAppendRelativeImmed(state);
    case Mmx_Gd_Operand:
    case Mmx_G_Operand:
      return NaClAppendOperandReg(state, operand, NaClGetGenRegRegister(state),
                                  ModRmMmx);
    case Mmx_E_Operand:
      return NaClAppendEffectiveAddress(state, operand, ModRmMmx);
    case Xmm_G_Operand:
    case Xmm_Go_Operand:
      return NaClAppendOperandReg(state, operand, NaClGetGenRegRegister(state),
                                  ModRmXmm);
    case Xmm_E_Operand:
    case Xmm_Eo_Operand:
      return NaClAppendEffectiveAddress(state, operand, ModRmXmm);
    case C_Operand:
      return NaClAppendEffectiveAddress(state, operand, ModRmCreg);
    case D_Operand:
      return NaClAppendEffectiveAddress(state, operand, ModRmDreg);
    case O_Operand:
    case Ob_Operand:
    case Ow_Operand:
    case Ov_Operand:
    case Oo_Operand:
      return NaClAppendMemoryOffsetImmed(state);
    case St_Operand:
      return NaClAppendStOpcodeBaseReg(state);
    case RegUnknown:
    case RegAL:
    case RegBL:
    case RegCL:
    case RegDL:
    case RegAH:
    case RegBH:
    case RegCH:
    case RegDH:
    case RegDIL:
    case RegSIL:
    case RegBPL:
    case RegSPL:
    case RegR8B:
    case RegR9B:
    case RegR10B:
    case RegR11B:
    case RegR12B:
    case RegR13B:
    case RegR14B:
    case RegR15B:
    case RegAX:
    case RegBX:
    case RegCX:
    case RegDX:
    case RegSI:
    case RegDI:
    case RegBP:
    case RegSP:
    case RegR8W:
    case RegR9W:
    case RegR10W:
    case RegR11W:
    case RegR12W:
    case RegR13W:
    case RegR14W:
    case RegR15W:
    case RegEAX:
    case RegEBX:
    case RegECX:
    case RegEDX:
    case RegESI:
    case RegEDI:
    case RegEBP:
    case RegESP:
    case RegR8D:
    case RegR9D:
    case RegR10D:
    case RegR11D:
    case RegR12D:
    case RegR13D:
    case RegR14D:
    case RegR15D:
    case RegCS:
    case RegDS:
    case RegSS:
    case RegES:
    case RegFS:
    case RegGS:
    case RegCR0:
    case RegCR1:
    case RegCR2:
    case RegCR3:
    case RegCR4:
    case RegCR5:
    case RegCR6:
    case RegCR7:
    case RegCR8:
    case RegCR9:
    case RegCR10:
    case RegCR11:
    case RegCR12:
    case RegCR13:
    case RegCR14:
    case RegCR15:
    case RegDR0:
    case RegDR1:
    case RegDR2:
    case RegDR3:
    case RegDR4:
    case RegDR5:
    case RegDR6:
    case RegDR7:
    case RegDR8:
    case RegDR9:
    case RegDR10:
    case RegDR11:
    case RegDR12:
    case RegDR13:
    case RegDR14:
    case RegDR15:
    case RegEFLAGS:
    case RegRFLAGS:
    case RegEIP:
    case RegRIP:
    case RegRAX:
    case RegRBX:
    case RegRCX:
    case RegRDX:
    case RegRSI:
    case RegRDI:
    case RegRBP:
    case RegRSP:
    case RegR8:
    case RegR9:
    case RegR10:
    case RegR11:
    case RegR12:
    case RegR13:
    case RegR14:
    case RegR15:
    case RegST0:
    case RegST1:
    case RegST2:
    case RegST3:
    case RegST4:
    case RegST5:
    case RegST6:
    case RegST7:
    case RegMMX1:
    case RegMMX2:
    case RegMMX3:
    case RegMMX4:
    case RegMMX5:
    case RegMMX6:
    case RegMMX7:
    case RegXMM0:
    case RegXMM1:
    case RegXMM2:
    case RegXMM3:
    case RegXMM4:
    case RegXMM5:
    case RegXMM6:
    case RegXMM7:
    case RegXMM8:
    case RegXMM9:
    case RegXMM10:
    case RegXMM11:
    case RegXMM12:
    case RegXMM13:
    case RegXMM14:
    case RegXMM15:
      return NaClAppendReg(operand->kind, &state->nodes);
    case RegREIP:
      return NaClAppendReg(state->address_size == 64 ? RegRIP : RegEIP,
                           &state->nodes);

    case RegREAX:
      return NaClAppendBasedOnSize(RegAX, RegEAX, RegRAX, state);
    case RegREBX:
      return NaClAppendBasedOnSize(RegBX, RegEBX, RegRBX, state);
    case RegRECX:
      return NaClAppendBasedOnSize(RegCX, RegECX, RegRCX, state);
    case RegREDX:
      return NaClAppendBasedOnSize(RegDX, RegEDX, RegRDX, state);
    case RegRESP:
      return NaClAppendBasedOnSize(RegSP, RegESP, RegRSP, state);
    case RegREBP:
      return NaClAppendBasedOnSize(RegBP, RegEBP, RegRBP, state);
    case RegRESI:
      return NaClAppendBasedOnSize(RegSI, RegESI, RegRSI, state);
    case RegREDI:
      return NaClAppendBasedOnSize(RegDI, RegEDI, RegRSI, state);
    case RegREAXa:
      return NaClAppendBasedOnAddressSize(RegAX, RegEAX, RegRAX, state);

    case RegDS_EDI:
      return NaClAppendDS_EDI(state);
    case RegDS_EBX:
      return NaClAppendDS_EBX(state);
    case RegES_EDI:
      return NaClAppendES_EDI(state);

    case S_Operand:
      return NaClAppendModRmSegmentReg(state);

    case Const_1:
      return NaClAppendConst(1,
                            NACL_EFLAG(ExprSize8) | NACL_EFLAG(ExprUnsignedHex),
                            &state->nodes);
    default:
      /* Give up, use the default of undefined. */
      break;
  }
  return NaClFatal("Operand", state);
}

/* Given that the given expression node is the root of the expression
 * tree generated by translating the given operand, transfer over
 * any appropriate flags (such as set/use information).
 */
static NaClExp* NaClAddOpSetUse(NaClExp* node, const NaClOp* operand) {
  if (operand->flags & NACL_OPFLAG(OpSet)) {
    node->flags |= NACL_EFLAG(ExprSet);
  }
  if (operand->flags & NACL_OPFLAG(OpDest)) {
    node->flags |= NACL_EFLAG(ExprDest);
  }
  if (operand->flags & NACL_OPFLAG(OpUse)) {
    node->flags |= NACL_EFLAG(ExprUsed);
  }
  if (operand->flags & NACL_OPFLAG(OpAddress)) {
    node->flags |= NACL_EFLAG(ExprAddress);
  }
  return node;
}

void NaClBuildExpVector(struct NaClInstState* state) {
  uint8_t i;
  uint8_t num_ops;
  DEBUG(NaClLog(LOG_INFO,
                "building expression vector for pc = %"NACL_PRIxNaClPcAddress
                ":\n",
                NaClInstStateVpc(state)));
  num_ops = NaClGetInstNumberOperandsInline(state->inst);
  for (i = 0; i < num_ops; i++) {
    NaClExp* n;
    const NaClOp* op = NaClGetInstOperandInline(state->decoder_tables,
                                                state->inst, i);
    DEBUG(NaClLog(LOG_INFO, "translating operand %d:\n", i));
    n = NaClAppendExp(OperandReference, i, 0, &state->nodes);
    if (op->flags & NACL_OPFLAG(OpImplicit)) {
      n->flags |= NACL_EFLAG(ExprImplicit);
    }
    NaClAddOpSetUse(NaClAppendOperand(state, op), op);
    DEBUG(NaClExpVectorPrint(NaClLogGetGio(), &state->nodes));
  }
}
