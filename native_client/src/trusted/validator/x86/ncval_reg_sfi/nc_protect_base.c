/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* nc_protect_base.h - For 64-bit mode, verifies that no instruction
 * changes the value of the base register, that the invariant between
 * RSP and RBP is maintained, and that segment registers are not set.
 */
#include <assert.h>

#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_protect_base.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_state_internal.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_trans.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter_internal.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_utils.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_jumps.h"

/* To turn on debugging of instruction decoding, change value of
 * DEBUGGING to 1.
 */
#define DEBUGGING 0

#include "native_client/src/shared/utils/debugging.h"

#include "native_client/src/trusted/validator/x86/decoder/ncopcode_desc_inl.c"
#include "native_client/src/trusted/validator/x86/decoder/ncop_exps_inl.c"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_iter_inl.c"

/* Defines locals used by the NaClBaseRegisterValidator to
 * record registers set in the current instruction, that are
 * a problem if not used correctly in the next instruction.
 */
typedef struct NaClRegisterLocals {
  /* Points to an instruction that contains an assignment to register ESP,
   * or NULL if the instruction doesn't set ESP. This is done so that we
   * can check if the next instruction uses the value of ESP to update RSP
   * (if not, we need to report that ESP is incorrectly assigned).
   */
  NaClInstState* esp_set_inst;
  /* Points to the instruction that contains an assignment to register EBP,
   * or NULL if the instruction doesn't set EBP. This is done so that we
   * can check if the next instruciton uses the value of EBP to update RBP
   * (if not, we need to report that EBP is incorrectly assigned).
   */
  NaClInstState* ebp_set_inst;
} NaClRegisterLocals;

/* Ths size of the circular buffer, used to keep track of registers
 * assigned in the previous instruction, that must be correctly used
 * in the current instruction, or reported as an error.
 */
#define NACL_REGISTER_LOCALS_BUFFER_SIZE 2

/* A circular buffer of two elements, used to  keep track of the
 * current/previous instruction.
 */
typedef struct NaClBaseRegisterLocals {
  NaClRegisterLocals buffer[NACL_REGISTER_LOCALS_BUFFER_SIZE];
  int previous_index;
  int current_index;
} NaClBaseRegisterLocals;

static void NaClReportIllegalChangeToRsp(NaClValidatorState* state,
                                         NaClInstState* inst) {
  NaClValidatorInstMessage(LOG_ERROR, state, inst,
                           "Illegal assignment to RSP\n");
}

/* Checks flags in the given locals, and reports any
 * previous instructions that were marked as bad.
 *
 * Parameters:
 *   state - The state of the validator.
 *   iter - The instruction iterator being used by the validator.
 *   locals - The locals used by validator NaClBaseRegisterValidator.
 */
static void NaClMaybeReportPreviousBad(NaClValidatorState* state,
                                       NaClBaseRegisterLocals* locals) {
  NaClInstState* prev_esp_set_inst =
      locals->buffer[locals->previous_index].esp_set_inst;
  NaClInstState* prev_ebp_set_inst =
      locals->buffer[locals->previous_index].ebp_set_inst;

  /* First check if previous register references are not followed
   * by acceptable instructions.
   */
  if (NULL != prev_esp_set_inst) {
    NaClValidatorInstMessage(LOG_ERROR,
                             state,
                             prev_esp_set_inst,
                             "Illegal assignment to ESP\n");
    locals->buffer[locals->previous_index].esp_set_inst = NULL;
  }
  if (NULL != prev_ebp_set_inst) {
    NaClValidatorInstMessage(LOG_ERROR,
                             state,
                             prev_ebp_set_inst,
                             "Illegal assignment to EBP\n");
    locals->buffer[locals->previous_index].ebp_set_inst = NULL;
  }

  /* Now advance the register recording by one instruction. */
  locals->previous_index = locals->current_index;
  locals->current_index = ((locals->current_index + 1)
                           % NACL_REGISTER_LOCALS_BUFFER_SIZE);
}

NaClBaseRegisterLocals* NaClBaseRegisterMemoryCreate(
    NaClValidatorState* state) {
  int i;
  NaClBaseRegisterLocals* locals = (NaClBaseRegisterLocals*)
      malloc(sizeof(NaClBaseRegisterLocals));
  if (NULL == locals) {
    NaClValidatorMessage(
        LOG_ERROR, state,
        "Out of memory, can't allocate NaClBaseRegisterLocals\n");
  } else {
    for (i = 0; i < NACL_REGISTER_LOCALS_BUFFER_SIZE; ++i) {
      locals->buffer[i].esp_set_inst = NULL;
      locals->buffer[i].ebp_set_inst = NULL;
    }
    locals->previous_index = 0;
    locals->current_index = 1;
  }
  return locals;
}

void NaClBaseRegisterMemoryDestroy(NaClValidatorState* state,
                                   NaClBaseRegisterLocals* locals) {
  free(locals);
}

/* Returns true if the instruction is of the form
 *   OP %esp, C
 * where OP in { add , sub } and C is a 32 bit constant.
 */
static Bool NaClIsAddOrSubBoundedConstFromEsp(NaClInstState* state) {
  const NaClInst* inst = NaClInstStateInst(state);
  NaClExpVector* vector = NaClInstStateExpVector(state);
  return (InstAdd == inst->name || InstSub == inst->name) &&
      2 == NaClGetInstNumberOperandsInline(inst) &&
      /* Note: Since the vector contains a list of operand expressions, the
       * first operand reference is always at index zero, and its first child
       * (where the register would be defined) is at index 1.
       */
      ExprRegister == vector->node[1].kind &&
      RegESP == NaClGetExpRegisterInline(&vector->node[1]) &&
      /* Note: Since the first subtree is a register operand, it uses
       * nodes 0 and 1 in the vector (node 0 is the operand reference, and
       * node 1 is its child defining a register value). The second operand
       * reference therefore lies at node 2, and if the operand is defined by
       * a 32 bit constant, it is the first kid of node 2, which is node 3.
       */
      ExprConstant == vector->node[3].kind;
}

/* Returns true iff the instruction of form "lea _, [%reg+%rbase*1]" */
static Bool NaClIsLeaAddressRegPlusRbase(NaClValidatorState* state,
                                         NaClInstState* inst_state,
                                         NaClOpKind reg) {
  const NaClInst* inst = NaClInstStateInst(inst_state);
  assert((RegRSP == reg) || (RegRBP == reg));
  if (InstLea == inst->name &&
      2 == NaClGetInstNumberOperandsInline(inst)) {
    NaClExpVector* vector = NaClInstStateExpVector(inst_state);
    int op2_index =
        NaClGetExpKidIndex(vector,
                           NaClGetNthExpKind(vector, OperandReference, 2),
                           0);
    NaClExp* op2 = &(vector->node[op2_index]);
    /* Only allow memory offset nodes with address size 64. */
    if (ExprMemOffset == op2->kind &&
        NACL_EMPTY_EFLAGS != (op2->flags & NACL_EFLAG(ExprSize64))) {
      int base_reg_index = op2_index + 1;
      NaClOpKind base_reg = NaClGetExpVectorRegister(vector, base_reg_index);
      if (base_reg == reg) {
        int index_reg_index =
            base_reg_index + NaClExpWidth(vector, base_reg_index);
        NaClOpKind index_reg =
            NaClGetExpVectorRegister(vector, index_reg_index);
        if (index_reg == state->base_register) {
          int scale_index =
              index_reg_index + NaClExpWidth(vector, index_reg_index);
          if (ExprConstant == vector->node[scale_index].kind &&
              (uint64_t)1 == NaClGetExpConstant(vector, scale_index)) {
            int disp_index = scale_index + NaClExpWidth(vector, scale_index);
            if (ExprConstant == vector->node[disp_index].kind &&
                (uint64_t)0 == NaClGetExpConstant(vector, disp_index)) {
              return TRUE;
            }
          }
        }
      }
    }
  }
  /* If reached, did not match. */
  return FALSE;
}

/* Checks for:
 *     mov %REG(32), ...
 *     lea %REG, [%REG+%rbase*1]
 *
 * where REG is the passed 64-bit register, and REG(32) is
 * the corresponding 32-bit retister.
 */

static Bool NaClAcceptRegMoveLea32To64(struct NaClValidatorState* state,
                                       struct NaClInstIter* iter,
                                       const NaClInst* inst,
                                       NaClOpKind reg) {
  NaClInstState* inst_state = state->cur_inst_state;
  assert((RegRSP == reg) || (RegRBP == reg));
  if (NaClOperandOneIsRegisterSet(inst_state, reg) &&
      NaClInstIterHasLookbackStateInline(iter, 1)) {
    NaClInstState* prev_inst = NaClInstIterGetLookbackStateInline(iter, 1);
    if (NaClAssignsRegisterWithZeroExtends(
            prev_inst, NaClGet32For64BitReg(reg)) &&
        NaClIsLeaAddressRegPlusRbase(state, inst_state, reg)) {
      DEBUG(const char* reg_name = NaClOpKindName(reg);
            printf("nc protect base for 'lea %s. [%s, rbase]'\n",
                   reg_name, reg_name));
      NaClMarkInstructionJumpIllegal(state, inst_state);
      return TRUE;
    }
  }
  return FALSE;
}

/* Check if assignments to stack register RSP is legal.
 *
 * Parameters are:
 *   state - The state of the validator.
 *   iter  - The instruction iterator (defining the current instruction).
 *   locals- Pointer to the instructions defining propagated registers.
 *   i     - The index of the node (in the expression tree) that
 *           assigns the RSP register.
 */
static void NaClCheckRspAssignments(struct NaClValidatorState* state,
                                    NaClInstIter* iter,
                                    NaClBaseRegisterLocals* locals,
                                    uint32_t i) {
  /*
   * Only allow one of:
   * (1) mov %rsp, %rbp
   *
   *     Note: maintains RSP/RBP invariant, since RBP was already
   *     meeting the invariant.
   * (2) %esp = zero extend 32-bit value
   *     OP %rsp, %rbase
   *
   *     where OP in { or , add }.
   * (3) An instruction that updates the stack a (small) bounded amount,
   *     and then does a memory access. This includes Push, Pop, Call,
   *
   *     Note that entering a function corresponds to the pattern:
   *        push %rpb
   *        mov %rbp, %rsp
   * (4) Allow stack updates of the form:
   *        OP %esp, C
   *        add %rsp, %rbase
   *     where OP is a operator in { add , sub },
   *     and C is a 32-bit constant.
   * (5) Allow "and $rsp, 0xXX" where 0xXX is an immediate 8 bit
   *     value that is negative. Used to realign the stack pointer.
   * (6) %esp = zero extend 32-bit value.
   *     lea %rsp, [%rsp+%rbase*1]
   *
   *     Same as (6), except that we use instructions prior to the
   *     pattern to do the add/subtract. Then let the result be
   *     (zero-extended) moved into ESP, and use the lea to fill
   *     in the top 32 bits of %rsp.
   *
   *     Note: We require the scale to be 1, and rbase be in
   *     the index position.
   *
   * Note: Cases 2, 4, 5, and 6 are maintaining the invariant that
   * the top half of RSP is the same as RBASE, and the lower half
   * of RBASE is zero. Case (2) does this by seting the bottom 32
   * bits with the first instruction (zeroing out the top 32 bits),
   * and then copies (via or or add) the top 32 bits of RBASE into RSP
   * (since the bottom 32 bits of RBASE are zero).
   * Case (4) maintains this by first clearing the top half
   * of RSP, and then setting the top half to match RBASE. Case (5)
   * maintains the variant because the constant is small
   * (-1 to -128) to that the invariant for $RSP (top half
   * is unchanged). Case 6 uses the addition in the address calculation
   * of lea to fill in the top 32 bits.
   */
  NaClInstState* inst_state = state->cur_inst_state;
  const NaClInst* inst = state->cur_inst;
  NaClMnemonic inst_name = inst->name;
  NaClExpVector* vector = state->cur_inst_vector;
  switch (inst_name) {
    case InstPush:
    case InstPop:
      /* Legal if index corresponds to the first (stack) argument.
       * Note: Since the vector contains a list of operand exrpessions,
       * the first operand reference is always at index zero, and its
       * first child (where the stack register is defined) is at index 1.
       */
      if (i == 1) return;
      break;
    case InstCall:
      /* Legal if index corresponds to the second (stack) argument.
       * Note: The first operand is an operand reference to the instruction
       * register. It consists of an operand reference at index zero,
       * and its first child (where the instruction registers is defined)
       * is at index 1. The node at index 2 is the operand reference to
       * the stack register, and its first child (where the stack register is
       * defined) is at index 3;
       */
      if (i == 3) return;
      break;
    case InstOr:
    case InstAdd:
      {
        /* case 2/4 (depending on instruction name). */
        if (NaClIsBinarySetUsingRegisters(
                state->decoder_tables,
                inst, inst_name, vector, RegRSP,
                state->base_register) &&
            NaClInstIterHasLookbackStateInline(iter, 1)) {
          NaClInstState* prev_inst =
              NaClInstIterGetLookbackStateInline(iter, 1);
          if (NaClAssignsRegisterWithZeroExtends(prev_inst, RegESP)
              || (inst_name == InstAdd &&
                  NaClIsAddOrSubBoundedConstFromEsp(prev_inst))) {
            /* Found that the assignment to ESP in previous instruction
             * is legal, so long as the two instructions are atomic.
             */
            DEBUG(printf("nc protect esp for zero extend, or or/add "
                         "constant\n"));
            NaClMarkInstructionJumpIllegal(state, inst_state);
            locals->buffer[locals->previous_index].esp_set_inst = NULL;
            return;
          }
        }
      }
      break;
    case InstLea:
      if (NaClAcceptRegMoveLea32To64(state, iter, inst, RegRSP)) {
        /* case 6. Found that the assignment to ESP in the previous
         * instruction is legal, so long as the two instrucitons
         * are atomic.
         */
        DEBUG(printf("nc protect esp for lea\n"));
        NaClMarkInstructionJumpIllegal(state, inst_state);
        locals->buffer[locals->previous_index].esp_set_inst = NULL;
        return;
      }
      break;
    case InstAnd:
      /* See if case 5: and $rsp, 0xXX */
      if (NaClInstStateLength(inst_state) == 4 &&
          NaClInstStateByte(inst_state, 0) == 0x48 &&
          NaClInstStateByte(inst_state, 1) == 0x83 &&
          NaClInstStateByte(inst_state, 2) == 0xe4 &&
          /* negative byte test: check if leftmost bit set. */
          (NaClInstStateByte(inst_state, 3) & 0x80)) {
        return;
      }
      /* Intentionally fall to the next case. */
    default:
      if (NaClIsMovUsingRegisters(state->decoder_tables,
                                  inst, vector, RegRSP, RegRBP)) {
        /* case (1) -- see above, matching
         *    mov %rsp, %rbp
         */
        return;
      }
      break;
  }
  /* If reached, assume that not a special case. */
  NaClReportIllegalChangeToRsp(state, inst_state);
}

/* Check if assignments to rbp resister is legal.
 *
 * Parameters are:
 *   state - The state of the validator.
 *   iter  - The instruction iterator (defining the current instruction).
 *   locals- Pointer to the instructions defining propagated registers.
 *   i     - The index of the node (in the expression tree) that
 *           assigns the RSP register.
 */
static void NaClCheckRbpAssignments(struct NaClValidatorState* state,
                                    NaClInstIter* iter,
                                    NaClBaseRegisterLocals* locals,
                                    uint32_t i) {
  /* (1) mov %rbp, %rsp
   *
   *     Note: maintains RSP/RBP invariant, since RSP was already
   *     meeting the invariant.
   *
   * (2) %ebp = zero extend 32-bit value.
   *     add %rbp, %rbase
   *
   *     Typical use in the exit from a fucntion, restoring RBP.
   *     The ... in the MOV is gotten from a stack pop in such
   *     cases. However, for long jumps etc., the value may
   *     be gotten from memory, or even a register.
   *
   * (3) %ebp = zero extend 32-bit value.
   *     lea %rbp, [%rbp+%rbase*1]
   *
   *     Same as (2), except that we use instructions prior to the
   *     pattern to do the add/subtract. Then let the result be
   *     (zero-extended) moved into EBP, and use the lea to fill
   *     in the top 32 bits of %RSP.
   *
   *     Note: We require the scale to be 1, and rbase be in
   *     the index position.
   */
  NaClInstState* inst_state = state->cur_inst_state;
  const NaClInst* inst = state->cur_inst;
  NaClMnemonic inst_name = inst->name;
  NaClExpVector* vector = state->cur_inst_vector;
  switch (inst_name) {
    case InstAdd:
      if (NaClInstIterHasLookbackStateInline(iter, 1)) {
        NaClInstState* prev_state =
            NaClInstIterGetLookbackStateInline(iter, 1);
        if (NaClIsBinarySetUsingRegisters(
                state->decoder_tables,
                inst, InstAdd, vector,
                RegRBP, state->base_register) &&
            NaClAssignsRegisterWithZeroExtends(
                prev_state, RegEBP)) {
          /* case 2. */
          NaClMarkInstructionJumpIllegal(state, inst_state);
          locals->buffer[locals->previous_index].ebp_set_inst = NULL;
          return;
        }
      }
      break;
    case InstLea:
      if (NaClAcceptRegMoveLea32To64(state, iter, inst, RegRBP)) {
        /* case 3 */
        locals->buffer[locals->previous_index].ebp_set_inst = NULL;
        return;
      }
      break;
    default:
      if (NaClIsMovUsingRegisters(inst_state->decoder_tables,
                                  inst, vector, RegRBP, RegRSP)) {
        /* case 1 */
        return;
      }
      break;
  }
  /* If reached, not valid. */
  NaClValidatorInstMessage(LOG_ERROR, state, inst_state,
                           "Illegal change to register RBP\n");
}

/* Reports error if the register name is a subregister of Rsp/Rbp/base register,
 * under assumption that it is illegal to change the value of such registers.
 */
static void NaClCheckSubregChangeOfRspRbpOrBase(
    struct NaClValidatorState* state,
    NaClOpKind reg_name) {
  NaClInstState* inst_state = state->cur_inst_state;
  if (NaClIs64Subreg(inst_state, reg_name, state->base_register)) {
    NaClValidatorInstMessage(
        LOG_ERROR, state, inst_state,
        "Changing %s changes the value of %s\n",
        NaClOpKindName(reg_name),
        NaClOpKindName(state->base_register));
  } else if (NaClIs64Subreg(inst_state, reg_name, RegRSP)) {
    NaClValidatorInstMessage(
        LOG_ERROR, state, inst_state,
        "Changing %s changes the value of %s\n",
        NaClOpKindName(reg_name),
        NaClOpKindName(RegRSP));
  } else if (NaClIs64Subreg(inst_state, reg_name, RegRBP)) {
    NaClValidatorInstMessage(
        LOG_ERROR, state, inst_state,
        "Changing %s changes the value of %s\n",
        NaClOpKindName(reg_name),
        NaClOpKindName(RegRBP));
  }
}

void NaClBaseRegisterValidator(struct NaClValidatorState* state,
                               struct NaClInstIter* iter,
                               NaClBaseRegisterLocals* locals) {
  uint32_t i;
  NaClInstState* inst_state = state->cur_inst_state;
  NaClExpVector* vector = state->cur_inst_vector;

  DEBUG(NaClValidatorInstMessage(
      LOG_INFO, state, inst_state, "Checking base registers...\n"));

  /* Look for assignments to registers. */
  for (i = 0; i < vector->number_expr_nodes; ++i) {
    NaClExp* node = &vector->node[i];
    if (ExprRegister == node->kind) {
      if (node->flags & NACL_EFLAG(ExprSet)) {
        NaClOpKind reg_name = NaClGetExpRegisterInline(node);

        /* If reached, found an assignment to a register.
         * Check if its one that we care about (i.e.
         * the base register (RBASE), RSP, RBP, or segment register).
         */
        if (reg_name == state->base_register) {
          NaClValidatorInstMessage(
              LOG_ERROR, state, inst_state,
              "Illegal to change the value of register %s\n",
              NaClOpKindName(state->base_register));
        } else {
          switch (reg_name) {
            case RegRSP:
              NaClCheckRspAssignments(state, iter, locals, i);
              break;
            case RegRBP:
              NaClCheckRbpAssignments(state, iter, locals, i);
              break;
            case RegESP:
              /* Record that we must recheck this after we have
               * moved to the next instruction.
               */
              locals->buffer[locals->current_index].esp_set_inst = inst_state;
              break;
            case RegEBP:
              /* Record that we must recheck this after we have
               * moved to the next instruction.
               */
              locals->buffer[locals->current_index].ebp_set_inst = inst_state;
              break;
            case RegCS:
            case RegDS:
            case RegSS:
            case RegES:
            case RegFS:
            case RegGS:
              NaClValidatorInstMessage(
                  LOG_ERROR, state, inst_state,
                  "Illegal assignment to segment register %s\n",
                  NaClOpKindName(reg_name));
              break;
            default:
              NaClCheckSubregChangeOfRspRbpOrBase(state, reg_name);
              break;
          }
        }
      }
    }
  }
  /* Before moving to the next instruction, see if we need to report
   * problems with the previous instruction.
   */
  NaClMaybeReportPreviousBad(state, locals);
}

void NaClBaseRegisterSummarize(struct NaClValidatorState* state,
                             struct NaClInstIter* iter,
                             struct NaClBaseRegisterLocals* locals) {
  /* Check if problems in last instruction of segment. */
  NaClMaybeReportPreviousBad(state, locals);
}
