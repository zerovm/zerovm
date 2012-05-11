/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/validator/x86/decoder/ncop_exps.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>

#include "include/portability.h"
#include "src/platform/nacl_log.h"
#include "src/utils/types.h"
#include "src/validator/x86/decoder/gen/ncop_expr_node_flag_impl.h"
#include "src/validator/x86/decoder/gen/ncop_expr_node_kind_impl.h"
#include "src/validator/x86/decoder/nc_decode_tables_types.h"
#include "src/validator/x86/decoder/nc_inst_state_internal.h"

/* To turn on debugging of instruction decoding, change value of
 * DEBUGGING to 1.
 *
 * WARNING: Debugging messages inside of print messages must be sent to the
 * same gio stream as being printed, since they may be used by another
 * nacl log message that has locked the access to NaClLogGetGio().
 */
#define DEBUGGING 0

#include "src/utils/debugging.h"

#include "src/validator/x86/decoder/ncopcode_desc_inl.c"
#include "src/validator/x86/decoder/ncop_exps_inl.c"

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
  {ExprSegmentAddress, 2},
  {ExprMemOffset, 4},
  {ExprNaClIllegal, 0},
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
                                    NaClInstState* state,
                                    uint32_t index);

/* Print the characters in the given string using lower case. */
static void NaClPrintLower(struct Gio* file, char* str) {
  while (*str) {
    gprintf(file, "%c", tolower(*str));
    ++str;
  }
}

/* Print out the given constant expression node to the given file. */
static void NaClPrintDisassembledConst(
    struct Gio* file, NaClInstState* state, NaClExp* node) {
  assert(node->kind == ExprConstant);
  if (node->flags & NACL_EFLAG(ExprJumpTarget)) {
    NaClPcAddress target = NaClInstStatePrintableAddress(state)
        + state->bytes.length + (NaClPcNumber) NaClGetExprSignedValue(node);
    gprintf(file, "0x%"NACL_PRIxNaClPcAddress, target);
  }else if (node->flags & NACL_EFLAG(ExprUnsignedHex)) {
    gprintf(file, "0x%"NACL_PRIx64, NaClGetExprUnsignedValue(node));
  } else if (node->flags & NACL_EFLAG(ExprSignedHex)) {
    int64_t val = NaClGetExprSignedValue(node);
    if (val < 0) {
      val = -val;
      gprintf(file, "-0x%"NACL_PRIx64, val);
    } else {
      gprintf(file, "0x%"NACL_PRIx64, val);
    }
  } else if (node->flags & NACL_EFLAG(ExprUnsignedInt)) {
    gprintf(file, "%"NACL_PRIu64, NaClGetExprUnsignedValue(node));
  } else {
    /* Assume ExprSignedInt. */
    gprintf(file, "%"NACL_PRId64, NaClGetExprSignedValue(node));
  }
}

#define NACLOP_REG_PREFIX "Reg"

size_t NaClOpRegName(NaClOpKind reg, char* buffer, size_t buffer_size) {
  const char* name = NaClOpKindName(reg);
  char* str;
  size_t index;

  /* Fail if no room to put register name. */
  if (buffer_size == 0) return 0;
  buffer[0] = '\0';  /* To be safe, in case we exit prematurely. */

  /* Get name for register. */
  name = NaClOpKindName(reg);
  if (NULL == name) return 0;

  /* Strip off 'Reg' prefix from register name, if it exists. */
  str = strstr(name, NACLOP_REG_PREFIX);
  if (str != name) return 0;
  str += strlen(NACLOP_REG_PREFIX);

  /* Copy the name, converting characters to lower case. */
  for (index = 0; (index + 1) < buffer_size; ++index) {
    char ch = tolower(str[index]);
    if ('\0' == ch) break;
    buffer[index] = tolower(str[index]);
  }

  /* Be sure to add null character at end. */
  buffer[index] = '\0';
  return index;
}

#define MAX_REGISTER_SIZE 256

/* Print out the disassembled representation of the given register
 * to the given file.
 */
static void NaClPrintDisassembledRegKind(struct Gio* file, NaClOpKind reg) {
  char buffer[MAX_REGISTER_SIZE];
  NaClOpRegName(reg, buffer, MAX_REGISTER_SIZE);
  gprintf(file, "%c%s", '%', buffer);
}

static INLINE void NaClPrintDisassembledReg(struct Gio* file, NaClExp* node) {
  NaClPrintDisassembledRegKind(file, NaClGetExpRegisterInline(node));
}

void NaClExpVectorPrint(struct Gio* file, NaClInstState* state) {
  uint32_t i;
  NaClExpVector* vector = NaClInstStateExpVector(state);
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
        NaClPrintDisassembledConst(file, state, node);
        break;
      default:
        gprintf(file, "%"NACL_PRIu64, NaClGetExprUnsignedValue(node));
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
                                      NaClInstState *state,
                                      int index) {
  NaClExpVector* vector = NaClInstStateExpVector(state);
  int r1_index = index + 1;
  int r2_index = r1_index + NaClExpWidth(vector, r1_index);
  int scale_index = r2_index + NaClExpWidth(vector, r2_index);
  int disp_index = scale_index + NaClExpWidth(vector, scale_index);
  NaClOpKind r1 = NaClGetExpVectorRegister(vector, r1_index);
  NaClOpKind r2 = NaClGetExpVectorRegister(vector, r2_index);
  uint64_t scale = NaClGetExprUnsignedValue(&vector->node[scale_index]);
  int64_t disp = NaClGetExprSignedValue(&vector->node[disp_index]);
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
    gprintf(file, "*%d", (uint32_t) scale);
  }
  if (disp != 0) {
    if ((r1 != RegUnknown || r2 != RegUnknown) &&
        !NaClIsExpNegativeConstant(vector, disp_index)) {
      gprintf(file, "+");
    }
    /* Recurse to handle print using format flags. */
    NaClPrintDisassembledExp(file, state, disp_index);
  } else if (r1 == RegUnknown && r2 == RegUnknown) {
    /* be sure to generate case: [0x0]. */
    NaClPrintDisassembledExp(file, state, disp_index);
  }
  gprintf(file, "]");
  return disp_index + NaClExpWidth(vector, disp_index);
}

/* Retrurns true if the segment register of the indexed segment address is DS,
 * and DS has been marked (by the instruction) as the default register
 * for the segment address.
 */
static Bool IsSegmentAddressDsRegPair(NaClInstState* state,
                                      int index) {
  NaClExpVector* vector = NaClInstStateExpVector(state);
  NaClExp* segment_address = &vector->node[index];
  NaClExp* segment_register =
      &vector->node[NaClGetExpKidIndex(vector, index, 0)];
  return NaClHasBit(segment_address->flags, NACL_EFLAG(ExprDSrCase)) &&
      (segment_register->kind == ExprRegister) &&
      (RegDS == NaClGetExpRegisterInline(segment_register));
}

/* Retrurns true if the segment register of the index segment address is ES,
 * and ES has been marked (by the instruction) as the default register
 * for the segment address.
 */
static Bool IsSegmentAddressEsRegPair(NaClInstState* state,
                                      int index) {
  NaClExpVector* vector = NaClInstStateExpVector(state);
  NaClExp* segment_address = &vector->node[index];
  NaClExp* segment_register =
      &vector->node[NaClGetExpKidIndex(vector, index, 0)];
  return NaClHasBit(segment_address->flags, NACL_EFLAG(ExprESrCase)) &&
      (segment_register->kind == ExprRegister) &&
      (RegES == NaClGetExpRegisterInline(segment_register));
}

/* Print out the given (segment address) expression node to the
 * given file. Returns the index of the node following the
 * given (indexed) segment address.
 */
static int NaClPrintDisassembledSegmentAddr(struct Gio* file,
                                            NaClInstState* state,
                                            int index) {
  int memory_address;
  NaClExpVector* vector = NaClInstStateExpVector(state);
  /* If segment register is default. If so, do not print. */
  if (IsSegmentAddressDsRegPair(state, index) ||
      IsSegmentAddressEsRegPair(state, index)) {
    /* Segment register matches default.  Don't print. */
  } else {
    /* Print the segment register associated with the segment address. */
    NaClPrintDisassembledExp(file, state, index + 1);
    gprintf(file, ":");
  }
  memory_address = NaClGetExpKidIndex(vector, index, 1);
  if (vector->node[memory_address].kind == ExprRegister) {
    /* Special case segment address, where the register corresponds to
     * a memory address. Print out the register in '[]' brackets to
     * communicate that it is a memory reference.
     */
    int result;
    gprintf(file, "[");
    result = NaClPrintDisassembledExp(file, state, memory_address);
    gprintf(file, "]");
    return result;
  } else {
    /* print out memory address associated with segment address. */
    return NaClPrintDisassembledExp(file, state, memory_address);
  }
}

/* Print out the given expression node to the given file.
 * Returns the index of the node following the given indexed
 * expression.
 */
static int NaClPrintDisassembledExp(struct Gio* file,
                                    NaClInstState* state,
                                    uint32_t index) {
  NaClExp* node;
  NaClExpVector* vector = NaClInstStateExpVector(state);
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
      return NaClPrintDisassembledExp(file, state, index + 1);
    case ExprConstant:
      NaClPrintDisassembledConst(file, state, node);
      return index + 1;
    case ExprSegmentAddress:
      return NaClPrintDisassembledSegmentAddr(file, state, index);
    case ExprMemOffset:
      return NaClPrintDisassembledMemOffset(file, state, index);
    case ExprNaClIllegal:
      gprintf(file, "*NaClIllegal*");
      return index + 1;
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
                                     NaClInstState* state,
                                     NaClExpVector* vector,
                                     int seg_addr_index) {
  int seg_index = seg_addr_index + 1;
  if (*is_first) {
    gprintf(file, " ");
    *is_first = FALSE;
  } else {
    gprintf(file, ", ");
  }
  NaClPrintDisassembledExp(file, state, seg_index);
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
      NaClPrintDisassembledExp(file, state, tree_index);

      /* If this is a partial instruction, add set/use information
       * so that that it is more clear what was matched.
       */
      if (NaClHasBit(inst->flags, NACL_IFLAG(PartialInstruction)) &&
          node->kind == OperandReference) {
        const NaClOp* op =
            NaClGetInstOperandInline(state->decoder_tables,
                                     inst,
                                     (uint8_t) NaClGetExprUnsignedValue(node));
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
                                     ExprDSrCase, RegDS)) {
            NaClPrintSegmentOverride(file, &is_first, state, vector,
                                     seg_addr_index);
          } else if (NaClHasSegmentOverride(vector, seg_addr_index,
                                            ExprESrCase, RegES)) {
            NaClPrintSegmentOverride(file, &is_first, state, vector,
                                     seg_addr_index);
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
  DEBUG(NaClExpVectorPrint(file, state));
  gprintf(file, "%"NACL_PRIxNaClPcAddressAll": ",
          NaClInstStatePrintableAddress(state));
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

/* Defines a buffer size big enough to hold an instruction. */
#define MAX_INST_TEXT_SIZE 256

char* NaClInstStateInstructionToString(struct NaClInstState* state) {
  /* Print to a memory buffer, and then duplicate. */
  struct GioMemoryFile filemem;
  struct Gio *file = (struct Gio*) &filemem;
  char buffer[MAX_INST_TEXT_SIZE];
  char* result;

  /* Note: Be sure to leave an extra byte to add the null character to
   * the end of the string.
   */
  GioMemoryFileCtor(&filemem, buffer, MAX_INST_TEXT_SIZE - 1);
  NaClInstStateInstPrint(file, state);
  buffer[filemem.curpos < MAX_INST_TEXT_SIZE
         ? filemem.curpos : MAX_INST_TEXT_SIZE] ='\0';
  result = strdup(buffer);
  GioMemoryFileDtor(file);
  return result;
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

Bool NaClIsExpNegativeConstant(NaClExpVector* vector, int index) {
  NaClExp* node = &vector->node[index];
  switch (node->kind) {
    case ExprConstant:
      if (node->flags & NACL_EFLAG(ExprUnsignedHex) ||
          node->flags & NACL_EFLAG(ExprUnsignedInt)) {
        return FALSE;
      } else {
        /* Assume signed value. */
        return NaClGetExprSignedValue(node) < 0;
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
