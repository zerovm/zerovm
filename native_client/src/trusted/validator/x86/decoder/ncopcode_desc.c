/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Descriptors to model instructions, opcodes, and instruction operands. */

#include "native_client/src/trusted/validator/x86/decoder/ncopcode_desc.h"

#include <assert.h>
#include <string.h>

#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/validator/x86/decoder/gen/ncopcode_prefix_impl.h"
#include "native_client/src/trusted/validator/x86/decoder/gen/ncopcode_insts_impl.h"
#include "native_client/src/trusted/validator/x86/decoder/gen/ncopcode_opcode_flags_impl.h"
#include "native_client/src/trusted/validator/x86/decoder/gen/ncopcode_operand_kind_impl.h"
#include "native_client/src/trusted/validator/x86/decoder/gen/ncopcode_operand_flag_impl.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_decode_tables.h"

#include "native_client/src/trusted/validator/x86/decoder/ncopcode_desc_inl.c"

uint8_t NaClGetOpcodeInModRm(uint8_t opcode_ext) {
  return opcode_ext & 0x0F;
}

uint8_t NaClGetOpcodeInModRmRm(uint8_t opcode_ext) {
  return (opcode_ext >> 4) & 0x0F;
}

uint8_t NaClGetOpcodePlusR(uint8_t opcode_ext) {
  return opcode_ext & 0x0F;
}

uint8_t NaClGetInstNumberOperands(const NaClInst* inst) {
  return NaClGetInstNumberOperandsInline(inst);
}

const NaClOp* NaClGetInstOperand(const NaClDecodeTables* tables,
                                 const NaClInst* inst, uint8_t index) {
  return NaClGetInstOperandInline(tables, inst, index);
}

/* Print out the opcode operand flags in a simplified (i.e. more human readable)
 * form.
 */
void NaClOpFlagsPrint(struct Gio* f, NaClOpFlags flags) {
  NaClOpFlag i;
  Bool first = TRUE;
  for (i = 0; i < NaClOpFlagEnumSize; ++i) {
    if (flags & NACL_OPFLAG(i)) {
      if (first) {
        first = FALSE;
      } else {
        gprintf(f, " ");
      }
      gprintf(f, "%s", NaClOpFlagName(i));
    }
  }
}

/* Print out the opcode operand in a simplified (i.e. more human readable)
 * form.
 */
void NaClOpPrint(struct Gio* f, const NaClOp* operand) {
  gprintf(f, "%s", NaClOpKindName(operand->kind));
  if (operand->flags) {
    size_t i;
    for (i = strlen(NaClOpKindName(operand->kind)); i < 24; ++i) {
      gprintf(f, " ");
    }
    NaClOpFlagsPrint(f, operand->flags);
  }
  gprintf(f, "\n");
}

/* Print instruction flags using a simplified (i.e. more human readable) form */
void NaClIFlagsPrint(struct Gio* f, NaClIFlags flags) {
  int i;
  Bool first = TRUE;
  for (i = 0; i < NaClIFlagEnumSize; ++i) {
    if (flags & NACL_IFLAG(i)) {
      if (first) {
        first = FALSE;
      } else {
        gprintf(f, " ");
      }
      gprintf(f, "%s", NaClIFlagName(i));
    }
  }
}

/* Returns a string defining bytes of the given prefix that are considered
 * prefix bytes, independent of the opcode.
 */
const char* OpcodePrefixBytes(NaClInstPrefix prefix) {
  switch(prefix) {
    case NoPrefix:
    case Prefix0F:
    case Prefix0F0F:
    case Prefix0F38:
    case Prefix0F3A:
    case PrefixD8:
    case PrefixD9:
    case PrefixDA:
    case PrefixDB:
    case PrefixDC:
    case PrefixDD:
    case PrefixDE:
    case PrefixDF:
    default:  /* This shouldn't happen, but be safe. */
      return "";
    case PrefixF20F:
    case PrefixF20F38:
      return "f2";
    case PrefixF30F:
      return "f3";
    case Prefix660F:
    case Prefix660F38:
    case Prefix660F3A:
      return "66";
  }
}
