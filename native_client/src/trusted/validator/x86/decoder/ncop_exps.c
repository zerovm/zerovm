/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/validator/x86/decoder/ncop_exps.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/validator/x86/decoder/gen/ncop_expr_node_flag_impl.h"
#include "native_client/src/trusted/validator/x86/decoder/gen/ncop_expr_node_kind_impl.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_decode_tables_types.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_state_internal.h"

/* To turn on debugging of instruction decoding, change value of
 * DEBUGGING to 1.
 *
 * WARNING: Debugging messages inside of print messages must be sent to the
 * same gio stream as being printed, since they may be used by another
 * nacl log message that has locked the access to NaClLogGetGio().
 */
#define DEBUGGING 0

#include "native_client/src/shared/utils/debugging.h"

#include "native_client/src/trusted/validator/x86/decoder/ncopcode_desc_inl.c"
#include "native_client/src/trusted/validator/x86/decoder/ncop_exps_inl.c"

void NaClPrintExpFlags(struct Gio* file, NaClExpFlags flags) {
  if (flags == 0) {
    gprintf(file, "0");
  } else {
    NaClExpFlag f;
    Bool is_first = TRUE;
    for (f = 0; f < NaClExpFlagEnumSize; f++) {
      if (flags & NACL_EFLAG(f)) {
        if (is_first) {
          is_first = FALSE;
        } else {
          gprintf(file, " | ");
        }
        gprintf(file, "%s", NaClExpFlagName(f));
      }
    }
  }
}

typedef struct NaClExpKindDescriptor {
  /* The name of the expression operator. */
  NaClExpKind name;
  /* The rank (i.e. number of children) the expression operator has. */
  const int rank;
} NaClExpKindDescriptor;

/* The print names of valid NaClExpKind values. */
static const NaClExpKindDescriptor
g_NaClExpKindDesc[NaClExpKindEnumSize + 1]= {
  {UndefinedExp, 0},
  {ExprRegister, 0},
  {OperandReference, 1},
  {ExprConstant, 0},
  {ExprConstant64, 2},
  {ExprSegmentAddress, 2},
  {ExprMemOffset, 4},
};

int NaClExpKindRank(NaClExpKind kind) {
  assert(kind == g_NaClExpKindDesc[kind].name);
  return g_NaClExpKindDesc[kind].rank;
}

/* Returns the register defined by the given node. */
NaClOpKind NaClGetExpRegister(NaClExp* node) {
  return NaClGetExpRegisterInline(node);
}

/* Returns the name of the register defined by the indexed node in the
 * vector of nodes.
 */
NaClOpKind NaClGetExpVectorRegister(NaClExpVector* vector,
                                    int node) {
  return NaClGetExpRegisterInline(&vector->node[node]);
}

static int NaClPrintDisassembledExp(struct Gio* file,
                                    NaClExpVector* vector,
                                    uint32_t index);

/* Print the characters in the given string using lower case. */
static void NaClPrintLower(struct Gio* file, char* str) {
  while (*str) {
    gprintf(file, "%c", tolower(*str));
    ++str;
  }
}

/* Return the sign (extended) integer in the given expr node. */
static int32_t NaClGetSignExtendedValue(NaClExp* node) {
  if (node->flags & NACL_EFLAG(ExprSize8)) {
    return (int8_t) node->value;
  } else if (node->flags & NACL_EFLAG(ExprSize16)) {
    return (int16_t) node->value;
  } else {
    return (int32_t) node->value;
  }
}

/* Print out the given (constant) expression node to the given file. */
static void NaClPrintDisassembledConst(struct Gio* file, NaClExp* node) {
  assert(node->kind == ExprConstant);
  if (node->flags & NACL_EFLAG(ExprUnsignedHex)) {
    gprintf(file, "0x%"NACL_PRIx32, node->value);
  } else if (node->flags & NACL_EFLAG(ExprSignedHex)) {
    int32_t value = NaClGetSignExtendedValue(node);
    if (value < 0) {
      value = -value;
      gprintf(file, "-0x%"NACL_PRIx32, value);
    } else {
      gprintf(file, "0x%"NACL_PRIx32, value);
    }
  } else if (node->flags & NACL_EFLAG(ExprUnsignedInt)) {
    gprintf(file, "%"NACL_PRIu32, node->value);
  } else {
    /* Assume ExprSignedInt. */
    gprintf(file, "%"NACL_PRId32, (int32_t) NaClGetSignExtendedValue(node));
  }
}

/* Print out the given (64-bit constant) expression node to the given file. */
static void NaClPrintDisassembledConst64(
    struct Gio* file, NaClExpVector* vector, int index) {
  NaClExp* node;
  uint64_t value;
  node = &vector->node[index];
  assert(node->kind == ExprConstant64);
  value = NaClGetExpConstant(vector, index);
  if (node->flags & NACL_EFLAG(ExprUnsignedHex)) {
    gprintf(file, "0x%"NACL_PRIx64, value);
  } else if (node->flags & NACL_EFLAG(ExprSignedHex)) {
    int64_t val = (int64_t) value;
    if (val < 0) {
      val = -val;
      gprintf(file, "-0x%"NACL_PRIx64, val);
    } else {
      gprintf(file, "0x%"NACL_PRIx64, val);
    }
  } else if (node->flags & NACL_EFLAG(ExprUnsignedInt)) {
    gprintf(file, "%"NACL_PRIu64, value);
  } else {
    gprintf(file, "%"NACL_PRId64, (int64_t) value);
  }
}

/* Print out the disassembled representation of the given register
 * to the given file.
 */
static void NaClPrintDisassembledRegKind(struct Gio* file, NaClOpKind reg) {
  const char* name = NaClOpKindName(reg);
  char* str = strstr(name, "Reg");
  gprintf(file, "%c", '%');
  NaClPrintLower(file, str == NULL ? (char*) name : str + strlen("Reg"));
}

static INLINE void NaClPrintDisassembledReg(struct Gio* file, NaClExp* node) {
  NaClPrintDisassembledRegKind(file, NaClGetExpRegisterInline(node));
}

void NaClExpVectorPrint(struct Gio* file, NaClExpVector* vector) {
  uint32_t i;
  gprintf(file, "NaClExpVector[%d] = {\n", vector->number_expr_nodes);
  for (i = 0; i < vector->number_expr_nodes; i++) {
    NaClExp* node = &vector->node[i];
    gprintf(file, "  { %s[%d] , ",
            NaClExpKindName(node->kind),
            NaClExpKindRank(node->kind));
    switch (node->kind) {
      case ExprRegister:
        NaClPrintDisassembledReg(file, node);
        break;
      case ExprConstant:
        NaClPrintDisassembledConst(file, node);
        break;
      case ExprConstant64:
        NaClPrintDisassembledConst64(file, vector, i);
        break;
      default:
        gprintf(file, "%"NACL_PRIu32, node->value);
        break;
    }
    gprintf(file, ", ");
    NaClPrintExpFlags(file, node->flags);
    gprintf(file, " },\n");
  }
  gprintf(file, "};\n");
}

/* Print out the given (memory offset) expression node to the given file.
 * Returns the index of the node following the given (indexed) memory offset.
 */
static int NaClPrintDisassembledMemOffset(struct Gio* file,
                                      NaClExpVector* vector,
                                      int index) {
  int r1_index = index + 1;
  int r2_index = r1_index + NaClExpWidth(vector, r1_index);
  int scale_index = r2_index + NaClExpWidth(vector, r2_index);
  int disp_index = scale_index + NaClExpWidth(vector, scale_index);
  NaClOpKind r1 = NaClGetExpVectorRegister(vector, r1_index);
  NaClOpKind r2 = NaClGetExpVectorRegister(vector, r2_index);
  int scale = (int) NaClGetExpConstant(vector, scale_index);
  uint64_t disp = NaClGetExpConstant(vector, disp_index);
  assert(ExprMemOffset == vector->node[index].kind);
  gprintf(file,"[");
  if (r1 != RegUnknown) {
    NaClPrintDisassembledRegKind(file, r1);
  }
  if (r2 != RegUnknown) {
    if (r1 != RegUnknown) {
      gprintf(file, "+");
    }
    NaClPrintDisassembledRegKind(file, r2);
    gprintf(file, "*%d", scale);
  }
  if (disp != 0) {
    if ((r1 != RegUnknown || r2 != RegUnknown) &&
        !NaClIsExpNegativeConstant(vector, disp_index)) {
      gprintf(file, "+");
    }
    /* Recurse to handle print using format flags. */
    NaClPrintDisassembledExp(file, vector, disp_index);
  } else if (r1 == RegUnknown && r2 == RegUnknown) {
    /* be sure to generate case: [0x0]. */
    NaClPrintDisassembledExp(file, vector, disp_index);
  }
  gprintf(file, "]");
  return disp_index + NaClExpWidth(vector, disp_index);
}

/* Print out the given (segment address) expression node to the
 * given file. Returns the index of the node following the
 * given (indexed) segment address.
 */
static int NaClPrintDisassembledSegmentAddr(struct Gio* file,
                                            NaClExpVector* vector,
                                            int index) {
  assert(ExprSegmentAddress == vector->node[index].kind);
  index = NaClPrintDisassembledExp(file, vector, index + 1);
  gprintf(file, ":");
  return NaClPrintDisassembledExp(file, vector, index);
}

/* Print out the given expression node to the given file.
 * Returns the index of the node following the given indexed
 * expression.
 */
static int NaClPrintDisassembledExp(struct Gio* file,
                                    NaClExpVector* vector,
                                    uint32_t index) {
  NaClExp* node;
  assert(index < vector->number_expr_nodes);
  node = &vector->node[index];
  switch (node->kind) {
    default:
      gprintf(file, "undefined");
      return index + 1;
    case ExprRegister:
      NaClPrintDisassembledReg(file, node);
      return index + 1;
    case OperandReference:
      return NaClPrintDisassembledExp(file, vector, index + 1);
    case ExprConstant:
      NaClPrintDisassembledConst(file, node);
      return index + 1;
    case ExprConstant64:
      NaClPrintDisassembledConst64(file, vector, index);
      return index + 3;
    case ExprSegmentAddress:
      return NaClPrintDisassembledSegmentAddr(file, vector, index);
    case ExprMemOffset:
      return NaClPrintDisassembledMemOffset(file, vector, index);
  }
}

/* Returns true if there is a segment override in the segment address
 * node defined by vector[seg_addr_index].
 *
 * Parameters:
 *   vector - The node expression tree associated with the instruction.
 *   seg_addr_index - The index to the segment address node to check.
 *   seg_eflag - The expr flag that must be associated with the
 *      segment address node to be considered for an override.
 *   seg_reg - The expected (i.e. default) segment register
 *      to be associated with the segment address.
 */
static Bool NaClHasSegmentOverride(NaClExpVector* vector,
                                   int seg_addr_index,
                                   NaClExpFlag seg_eflag,
                                   NaClOpKind seg_reg) {
  NaClExp* seg_node = &vector->node[seg_addr_index];
  if (seg_node->flags & NACL_EFLAG(seg_eflag)) {
    int seg_index = seg_addr_index + 1;
    NaClExp* node = &vector->node[seg_index];
    if ((ExprRegister == node->kind) &&
        (seg_reg != NaClGetExpRegisterInline(node))) {
      return TRUE;
    }
  }
  return FALSE;
}

/* Prints out the segment register associated with the segment
 * address node defined by vector[seg_addr_index].
 *
 * Parameters:
 *    file - The Gio file to print the segment register to.
 *    is_first - True if the first operand of the instruction.
 *    vector - The node expression tree associated with the instruction.
 *   seg_addr_index - The index to the segment address node to check.
 */
static void NaClPrintSegmentOverride(struct Gio* file,
                                     Bool* is_first,
                                     NaClExpVector* vector,
                                     int seg_addr_index) {
  int seg_index = seg_addr_index + 1;
  if (*is_first) {
    gprintf(file, " ");
    *is_first = FALSE;
  } else {
    gprintf(file, ", ");
  }
  NaClPrintDisassembledExp(file, vector, seg_index);
}

/* Print the flag name if the flag is defined for the corresponding operand.
 * Used to print out set/use/zero extend information for partial instructions.
 */
static void NaClPrintAddOperandFlag(struct Gio* f,
                                    const NaClOp* op,
                                    NaClOpFlag flag,
                                    const char* flag_name) {
  if (op->flags & NACL_OPFLAG(flag)) {
    gprintf(f, "%s", flag_name);
  }
}

/* Print the given instruction opcode of the give state, to the
 * given file.
 */
static void NaClPrintDisassembled(struct Gio* file,
                                  NaClInstState* state,
                                  const NaClInst* inst) {
  uint32_t tree_index = 0;
  Bool is_first = TRUE;
  Bool not_printed_prefix_segment = TRUE;
  NaClExp* node;
  NaClExpVector* vector = NaClInstStateExpVector(state);

  /* Print the name of the instruction. */
  if (NaClHasBit(inst->flags, NACL_IFLAG(PartialInstruction))) {
    /* Instruction has been simplified. Print out corresponding
     * hints to the reader, so that they know that the instruction
     * has been simplified.
     */
    gprintf(file, "[P] ");
    NaClPrintLower(file, (char*) NaClMnemonicName(inst->name));
    if (NaClHasBit(inst->flags, NACL_IFLAG(NaClIllegal))) {
      gprintf(file, "(illegal)");
    }
  } else {
    NaClPrintLower(file, (char*) NaClMnemonicName(inst->name));
  }

  /* Use the generated expression tree to print out (non-implicit) operands
   * of the instruction.
   */
  while (tree_index < vector->number_expr_nodes) {
    node = &vector->node[tree_index];
    if (node->kind != OperandReference ||
        (NACL_EMPTY_EFLAGS == (node->flags & NACL_EFLAG(ExprImplicit)))) {
      if (is_first) {
        gprintf(file, " ");
        is_first = FALSE;
      } else {
        gprintf(file, ", ");
      }
      NaClPrintDisassembledExp(file, vector, tree_index);

      /* If this is a partial instruction, add set/use information
       * so that that it is more clear what was matched.
       */
      if (NaClHasBit(inst->flags, NACL_IFLAG(PartialInstruction)) &&
          node->kind == OperandReference) {
        const NaClOp* op =
            NaClGetInstOperandInline(state->decoder_tables,
                                     inst, (uint8_t) node->value);
        if (NaClHasBit(op->flags, (NACL_OPFLAG(OpSet) |
                                   NACL_OPFLAG(OpUse) |
                                   NACL_OPFLAG(OperandZeroExtends_v)))) {
          gprintf(file, " (");
          NaClPrintAddOperandFlag(file, op, OpSet, "s");
          NaClPrintAddOperandFlag(file, op, OpUse, "u");
          NaClPrintAddOperandFlag(file, op, OperandZeroExtends_v, "z");
          gprintf(file, ")");
        }
      }
    } else if (not_printed_prefix_segment &&
               (OperandReference == node->kind) &&
               (node->flags & NACL_EFLAG(ExprImplicit))) {
      /* Print out segment override of implicit segment address, if
       * applicable.
       */
      if (OperandReference == node->kind) {
        int seg_addr_index = tree_index + 1;
        if (ExprSegmentAddress == vector->node[seg_addr_index].kind) {
          if (NaClHasSegmentOverride(vector, seg_addr_index,
                                     ExprDSrDICase, RegDS)) {
            NaClPrintSegmentOverride(file, &is_first, vector, seg_addr_index);
          } else if (NaClHasSegmentOverride(vector, seg_addr_index,
                                            ExprESrDICase, RegES)) {
            NaClPrintSegmentOverride(file, &is_first, vector, seg_addr_index);
          }
        }
      }
    }
    /* Skip over expression to next expresssion. */
    tree_index += NaClExpWidth(vector, tree_index);
  }
}

void NaClInstStateInstPrint(struct Gio* file, NaClInstState* state) {
  int i;
  const NaClInst* inst;

  /* Print out the address and the inst bytes. */
  int length = NaClInstStateLength(state);

  DEBUG_OR_ERASE(
      NaClInstPrint(file, state->decoder_tables, NaClInstStateInst(state)));
  DEBUG(NaClExpVectorPrint(file, NaClInstStateExpVector(state)));
  gprintf(file, "%"NACL_PRIxNaClPcAddressAll": ", NaClInstStateVpc(state));
  for (i = 0; i < length; ++i) {
    gprintf(file, "%02"NACL_PRIx8" ", NaClInstStateByte(state, i));
  }
  for (i = length; i < NACL_MAX_BYTES_PER_X86_INSTRUCTION; ++i) {
    gprintf(file, "   ");
  }

  /* Print out the assembly instruction it disassembles to. */
  inst = NaClInstStateInst(state);
  NaClPrintDisassembled(file, state, inst);
  gprintf(file, "\n");
}

char* NaClInstStateInstructionToString(struct NaClInstState* state) {
  struct GioFile gfile;
  char* out_string;
  struct stat st;
  size_t file_size;
  size_t fread_items;
  FILE* file = NULL;
  struct Gio* g = NULL;

  do {
    file = fopen("out_file", "w");
    if (file == NULL) break;

    g = (struct Gio*) &gfile;
    if (0 == GioFileRefCtor(&gfile, file)) {
      GioFileDtor(g);
      g = NULL;
      break;
    }

#if NACL_LINUX || NACL_OSX
    chmod("out_file", S_IRUSR | S_IWUSR);
#endif

    NaClInstStateInstPrint(g, state);
    GioFileClose(g);

    if (stat("out_file", &st)) break;

    file_size = (size_t) st.st_size;
    if (file_size == 0) break;

    out_string = (char*) malloc(file_size + 1);
    if (out_string == NULL) break;

    file = fopen("out_file", "r");
    if (file == NULL) break;

    fread_items = fread(out_string, file_size, 1, file);
    if (fread_items != 1) break;

    fclose(file);
    unlink("out_file");
    out_string[file_size] = 0;
    return out_string;
  } while (0);

  /* failure */
  if (g != NULL) {
    GioFileClose(g);
    file = NULL;
  }
  if (file != NULL) fclose(file);
  unlink("out_file");
  return NULL;
}

int NaClExpWidth(NaClExpVector* vector, int node) {
  int i;
  int count = 1;
  int num_kids = NaClExpKindRank(vector->node[node].kind);
  for (i = 0; i < num_kids; i++) {
    count += NaClExpWidth(vector, node + count);
  }
  return count;
}

int NaClGetExpKidIndex(NaClExpVector* vector, int node, int kid) {
  node++;
  while (kid-- > 0) {
    node += NaClExpWidth(vector, node);
  }
  return node;
}

int NaClGetExpParentIndex(NaClExpVector* vector, int index) {
  int node_rank;
  int num_kids = 1;
  while (index > 0) {
    --index;
    node_rank = NaClExpKindRank(vector->node[index].kind);
    if (node_rank >= num_kids) {
      return index;
    } else {
      num_kids -= (node_rank - 1);
    }
  }
  return -1;
}

int NaClGetNthExpKind(NaClExpVector* vector,
                      NaClExpKind kind,
                      int n) {
  if (n > 0) {
    uint32_t i;
    for (i = 0; i < vector->number_expr_nodes; ++i) {
      if (kind == vector->node[i].kind) {
        --n;
        if (n == 0) return i;
      }
    }
  }
  return -1;
}

uint64_t NaClGetExpConstant(NaClExpVector* vector, int index) {
  NaClExp* node = &vector->node[index];
  switch (node->kind) {
    case ExprConstant:
      return node->value;
    case ExprConstant64:
      return (uint64_t) (uint32_t) vector->node[index+1].value |
          (((uint64_t) vector->node[index+2].value) << 32);
    default:
      assert(0);
  }
  /* NOT REACHED */
  return 0;
}

void NaClSplitExpConstant(uint64_t val, uint32_t* val1, uint32_t* val2) {
  *val1 = (uint32_t) (val & 0xFFFFFFFF);
  *val2 = (uint32_t) (val >> 32);
}

Bool NaClIsExpNegativeConstant(NaClExpVector* vector, int index) {
  NaClExp* node = &vector->node[index];
  switch (node->kind) {
    case ExprConstant:
      if (node->flags & NACL_EFLAG(ExprUnsignedHex) ||
          node->flags & NACL_EFLAG(ExprUnsignedInt)) {
        return FALSE;
      } else {
        /* Assume signed value. */
        return NaClGetSignExtendedValue(node) < 0;
      }
      break;
    case ExprConstant64:
      if (node->flags & NACL_EFLAG(ExprUnsignedHex) ||
          node->flags & NACL_EFLAG(ExprUnsignedInt)) {
        return FALSE;
      } else {
        /* Assume signed value. */
        int64_t value = (int64_t) NaClGetExpConstant(vector, index);
        return value < 0;
      }
      break;
    default:
      break;
  }
  return FALSE;
}

/* Dummy routine to allow unreferenced NaClGetInstNumberOperandsInline
 * inline.
 */
uint8_t NaClNcopExpsDummyNaClGetInstNumberOperands(const NaClInst* inst) {
  return NaClGetInstNumberOperandsInline(inst);
}
