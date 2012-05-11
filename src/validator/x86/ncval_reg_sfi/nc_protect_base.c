/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* nc_protect_base.h - For 64-bit mode, verifies that no instruction
 * changes the value of the base register, that the invariant between
 * RSP and RBP is maintained, and that segment registers are not set.
 */
#include <assert.h>
#include <string.h>

#include "src/validator/x86/ncval_reg_sfi/nc_protect_base.h"

#include "include/portability_io.h"
#include "src/platform/nacl_log.h"
#include "src/validator/x86/decoder/ncop_exps.h"
#include "src/validator/x86/decoder/nc_inst_state_internal.h"
#include "src/validator/x86/decoder/nc_inst_trans.h"
#include "src/validator/x86/ncval_reg_sfi/ncvalidate_iter.h"
#include "src/validator/x86/ncval_reg_sfi/ncvalidate_iter_internal.h"
#include "src/validator/x86/ncval_reg_sfi/ncvalidate_utils.h"
#include "src/validator/x86/ncval_reg_sfi/nc_jumps.h"

/* To turn on debugging of instruction decoding, change value of
 * DEBUGGING to 1.
 */
#define DEBUGGING 0

#include "src/utils/debugging.h"

#include "src/validator/x86/decoder/ncopcode_desc_inl.c"
#include "src/validator/x86/decoder/ncop_exps_inl.c"
#include "src/validator/x86/decoder/nc_inst_iter_inl.c"

/* Maximum character buffer size to use for generating messages. */
static const size_t kMaxBufferSize = 1024;

static void NaClReportIllegalChangeToRsp(NaClValidatorState* state,
                                         NaClInstState* inst) {
  NaClValidatorInstMessage(LOG_ERROR, state, inst,
                           "Illegal assignment to RSP\n");
}

/* Checks flags in the possible set base registers, and reports any
 * previous instructions that were marked as bad.
 *
 * Parameters:
 *   state - The state of the validator.
 */
static INLINE void NaClMaybeReportPreviousBad(NaClValidatorState* state) {
  NaClInstState* prev_esp_set_inst =
      state->set_base_registers.buffer[
          state->set_base_registers.previous_index].esp_set_inst;
  NaClInstState* prev_ebp_set_inst =
      state->set_base_registers.buffer[
          state->set_base_registers.previous_index].ebp_set_inst;

  /* First check if previous register references are not followed
   * by acceptable instructions.
   */
  if (NULL != prev_esp_set_inst) {
    NaClValidatorInstMessage(LOG_ERROR,
                             state,
                             prev_esp_set_inst,
                             "Illegal assignment to ESP\n");
    state->set_base_registers.buffer[
        state->set_base_registers.previous_index].esp_set_inst = NULL;
  }
  if (NULL != prev_ebp_set_inst) {
    NaClValidatorInstMessage(LOG_ERROR,
                             state,
                             prev_ebp_set_inst,
                             "Illegal assignment to EBP\n");
    state->set_base_registers.buffer[
        state->set_base_registers.previous_index].ebp_set_inst = NULL;
  }

  /* Now advance the register recording by one instruction. */
  state->set_base_registers.previous_index =
      state->set_base_registers.current_index;
  state->set_base_registers.current_index =
      ((state->set_base_registers.current_index + 1)
       % NACL_REGISTER_LOCALS_BUFFER_SIZE);
}

void NaClBaseRegisterMemoryInitialize(NaClValidatorState* state) {
  int i;
  for (i = 0; i < NACL_REGISTER_LOCALS_BUFFER_SIZE; ++i) {
    state->set_base_registers.buffer[i].esp_set_inst = NULL;
    state->set_base_registers.buffer[i].ebp_set_inst = NULL;
  }
  state->set_base_registers.previous_index = 0;
  state->set_base_registers.current_index = 1;
}

/* Returns true iff the instruction of form "lea _, [%reg+%rbase*1]" */
static Bool NaClIsLeaAddressRegPlusRbase(NaClValidatorState* state,
                                         NaClInstState* inst_state,
                                         NaClOpKind reg) {
  const NaClInst* inst = NaClInstStateInst(inst_state);
  Bool result = FALSE;
  DEBUG(NaClLog(LOG_INFO, "-> LeaAddressRegPlusRbase(%s)\n",
                NaClOpKindName(reg)));
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
      DEBUG(NaClLog(LOG_INFO, "  base_reg = %s\n", NaClOpKindName(base_reg)));
      if (base_reg == reg) {
        int index_reg_index =
            base_reg_index + NaClExpWidth(vector, base_reg_index);
        NaClOpKind index_reg =
            NaClGetExpVectorRegister(vector, index_reg_index);
        DEBUG(NaClLog(LOG_INFO,
                      "  index_reg = %s\n", NaClOpKindName(index_reg)));
        if (index_reg == state->base_register) {
          int scale_index =
              index_reg_index + NaClExpWidth(vector, index_reg_index);
          DEBUG(NaClLog(LOG_INFO, "  scale_index = %d\n", scale_index));
          if (ExprConstant == vector->node[scale_index].kind) {
            if ((uint64_t)1 ==
                NaClGetExprUnsignedValue(&vector->node[scale_index])) {
              int disp_index = scale_index + NaClExpWidth(vector, scale_index);
              DEBUG(NaClLog(LOG_INFO, " disp_index = %d\n", disp_index));
              if (ExprConstant == vector->node[disp_index].kind) {
                if ((uint64_t)0 ==
                    NaClGetExprSignedValue(&vector->node[disp_index])) {
                  result = TRUE;
                } else {
                  DEBUG(NaClLog(LOG_INFO, "  disp not zero!\n"));
                }
              }
            } else {
              DEBUG(NaClLog(LOG_INFO, "  scale not 1!\n"));
            }
          }
        }
      }
    }
  }
  /* If reached, did not match. */
  DEBUG(NaClLog(LOG_INFO, "<-LeaAddressRegPlusRbase = %d\n", result));
  return result;
}

Bool NaClAcceptLeaWithMoveLea32To64(struct NaClValidatorState* state,
                                    NaClOpKind reg) {
  NaClInstState* inst_state = state->cur_inst_state;
  Bool result = NaClOperandOneIsRegisterSet(inst_state, reg) &&
      NaClIsLeaAddressRegPlusRbase(state, inst_state, reg) &&
      NaClAssignsRegisterWithZeroExtends64(state, 1, reg);
  if (result) {
    DEBUG({
        const char* reg_name = NaClOpKindName(reg);
        printf("nc protect base for 'lea %s. [%s, rbase]'\n",
               reg_name, reg_name);
      });
    result = TRUE;
  }
  return result;
}

/* Check if assignments to stack register RSP is legal.
 *
 * Parameters are:
 *   state - The state of the validator.
 *   i     - The index of the node (in the expression tree) that
 *           assigns the RSP register.
 */
static void NaClCheckRspAssignments(struct NaClValidatorState* state,
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
   *     Note: Since add/sub are zero-extending operations for operand
   *     size 32, this doesn't have to be treated as a special case!
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
#ifdef NCVAL_TESTING
  char* buffer;
  size_t buffer_size;
#endif

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
      /* case 2/4 (depending on instruction name) */
      if (NaClIsBinarySetUsingRegisters(
              state->decoder_tables,
              inst, inst_name, vector, RegRSP,
              state->base_register) &&
          NaClAssignsRegisterWithZeroExtends32(state, 1, RegESP)) {
#ifdef NCVAL_TESTING
        /* Report precondition of test. */
        NaClConditionAppend(state->precond, &buffer, &buffer_size);
        SNPRINTF(buffer, buffer_size, "ZeroExtends(esp)");
#endif
        NaClMarkInstructionJumpIllegal(state, state->cur_inst_state);
        state->set_base_registers.buffer[
            state->set_base_registers.previous_index].esp_set_inst = NULL;
        return;
      }
      break;
    case InstLea:
      if (NaClAcceptLeaWithMoveLea32To64(state, RegRSP)) {
        /* case 6. Found that the assignment to ESP in the previous
         * instruction is legal, so long as the two instructions
         * are atomic.
         */
#ifdef NCVAL_TESTING
        /* Report precondition of test. */
        NaClConditionAppend(state->precond, &buffer, &buffer_size);
        SNPRINTF(buffer, buffer_size, "ZeroExtends(esp)");
#endif
        NaClMarkInstructionJumpIllegal(state, state->cur_inst_state);
        state->set_base_registers.buffer[
            state->set_base_registers.previous_index].esp_set_inst = NULL;
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
 *   i     - The index of the node (in the expression tree) that
 *           assigns the RSP register.
 */
static void NaClCheckRbpAssignments(struct NaClValidatorState* state,
                                    uint32_t i) {
  /* (1) mov %rbp, %rsp
   *
   *     Note: maintains RSP/RBP invariant, since RSP was already
   *     meeting the invariant.
   *
   * (2) %ebp = zero extend 32-bit value.
   *     add %rbp, %rbase
   *
   *     Typical use in the exit from a function, restoring RBP.
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
#ifdef NCVAL_TESTING
  char* buffer;
  size_t buffer_size;
#endif

  switch (inst_name) {
    case InstAdd:
      /* case 2. */
      if (NaClIsBinarySetUsingRegisters(
              state->decoder_tables,
              inst, InstAdd, vector,
              RegRBP, state->base_register) &&
          NaClAssignsRegisterWithZeroExtends32(state, 1, RegEBP)) {
#ifdef NCVAL_TESTING
        /* Report precondition of test. */
        NaClConditionAppend(state->precond, &buffer, &buffer_size);
        SNPRINTF(buffer, buffer_size, "ZeroExtends(ebp)");
#endif
        NaClMarkInstructionJumpIllegal(state, state->cur_inst_state);
        state->set_base_registers.buffer[
            state->set_base_registers.previous_index].ebp_set_inst = NULL;
        return;
      }
      break;
    case InstLea:
      /* case 3 */
      if (NaClAcceptLeaWithMoveLea32To64(state, RegRBP)) {
#ifdef NCVAL_TESTING
        /* Report precondition of test. */
        NaClConditionAppend(state->precond, &buffer, &buffer_size);
        SNPRINTF(buffer, buffer_size, "ZeroExtends(ebp)");
#endif
        NaClMarkInstructionJumpIllegal(state, state->cur_inst_state);
        state->set_base_registers.buffer[
            state->set_base_registers.previous_index].ebp_set_inst = NULL;
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

void NaClBaseRegisterValidator(struct NaClValidatorState* state) {
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
              NaClCheckRspAssignments(state, i);
              break;
            case RegRBP:
              NaClCheckRbpAssignments(state, i);
              break;
            case RegESP:
              /* Record that we must recheck this after we have
               * moved to the next instruction.
               */
              state->set_base_registers.buffer[
                  state->set_base_registers.current_index
                                               ].esp_set_inst = inst_state;
              break;
            case RegEBP:
              /* Record that we must recheck this after we have
               * moved to the next instruction.
               */
              state->set_base_registers.buffer[
                  state->set_base_registers.current_index
                                               ].ebp_set_inst = inst_state;
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
  NaClMaybeReportPreviousBad(state);
}

void NaClBaseRegisterSummarize(struct NaClValidatorState* state) {
  /* Check if problems in last instruction of segment. */
  NaClMaybeReportPreviousBad(state);
}
