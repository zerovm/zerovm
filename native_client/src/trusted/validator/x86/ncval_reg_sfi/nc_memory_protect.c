/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_memory_protect.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_state.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_trans.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter_internal.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_utils.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_jumps.h"

/* To turn on debugging of instruction decoding, change value of
 * DEBUGGING to 1.
 */
#define DEBUGGING 0

#include "native_client/src/trusted/validator/x86/decoder/nc_inst_iter_inl.c"
#include "native_client/src/shared/utils/debugging.h"

#include "native_client/src/trusted/validator/x86/decoder/ncop_exps_inl.c"

/*
 * When true, check both uses and sets of memory. When false, only
 * check sets.
 */
Bool NACL_FLAGS_read_sandbox = TRUE;

/* Returns true if the node corresponds to an expression set, or an
 * expression use if we are doing read sandboxing.
 */
static Bool IsPossibleSandboxingNode(NaClExp* node) {
  return ((NACL_FLAGS_read_sandbox && (node->flags & NACL_EFLAG(ExprUsed))) ||
          (node->flags & NACL_EFLAG(ExprSet)));
}


/* Checks if the given node index for the given instruction is
 * a valid (sandboxed) memory offset, based on NACL rules, returning TRUE iff
 * the memory offset is NACL compliant.
 * parameters are:
 *    state - The state of the validator,
 *    iter - The current value of the instruction iterator.
 *    lookback_index - number of instructions to lookback
 *      (within the iterator) in order to retrieve the instruction.
 *    inst - The instruction that holds the memory offset.
 *    vector - The expression vector associated with the instruction.
 *    node_index - The index of the memory offset within the given
 *      instruction vector.
 *    print_messages - True if this routine is responsable for printing
 *      error messages if the memory offset isn't NACL compliant.
 */
static INLINE Bool NaClIsValidMemOffset(
    NaClValidatorState* state,
    NaClInstIter* iter,
    int lookback_index,
    NaClInstState* inst,
    NaClExpVector* vector,
    int node_index,
    Bool print_messages) {
  int base_reg_index;
  NaClOpKind base_reg;
  int index_reg_index;
  NaClExp* index_reg_node;
  NaClOpKind index_reg;
  int scale_index;
  int disp_index;
  NaClExp* node = &vector->node[node_index];

  if (ExprMemOffset != node->kind) return FALSE;
  DEBUG(NaClLog(LOG_INFO,
                "found MemOffset at node %"NACL_PRIu32"\n", node_index));
  /* Only allow memory offset nodes with address size 64. */
  if (NACL_EMPTY_EFLAGS == (node->flags & NACL_EFLAG(ExprSize64))) {
    if (print_messages) {
      NaClValidatorInstMessage(LOG_ERROR, state, inst,
                               "Assignment to non-64 bit memory address\n");
    }
    return FALSE;
  }
  DEBUG(NaClLog(LOG_INFO, "found 64 bit address for MemOffset\n"));
  base_reg_index = node_index + 1;
  base_reg = NaClGetExpVectorRegister(vector, base_reg_index);
  DEBUG(NaClLog(LOG_INFO, "base reg = %s\n", NaClOpKindName(base_reg)));
  if (base_reg != state->base_register &&
      base_reg != RegRSP &&
      base_reg != RegRBP &&
      base_reg != RegRIP) {
    if (print_messages) {
      NaClValidatorInstMessage(
          LOG_ERROR, state, inst,
          (base_reg == RegUnknown
           ? "No base register specified in memory offset\n"
           : "Invalid base register in memory offset\n"));
    }
    return FALSE;
  }
  DEBUG(NaClLog(LOG_INFO, "  => base register is valid\n"));
  index_reg_index = base_reg_index + NaClExpWidth(vector, base_reg_index);
  index_reg_node = &vector->node[index_reg_index];
  index_reg = NaClGetExpRegisterInline(index_reg_node);
  DEBUG(NaClLog(LOG_INFO, "index reg = %s\n", NaClOpKindName(index_reg)));
  if (RegUnknown != index_reg) {
    Bool index_reg_is_good = FALSE;
    if ((base_reg != RegRIP) &&
        (index_reg_node->flags & NACL_EFLAG(ExprSize64)) &&
        NaClInstIterHasLookbackStateInline(iter, lookback_index + 1)) {
      NaClInstState* prev_inst =
          NaClInstIterGetLookbackStateInline(iter, lookback_index + 1);
      DEBUG_OR_ERASE({
          struct Gio* g = NaClLogGetGio();
          NaClLog(LOG_INFO, "prev inst:\n");
          NaClInstPrint(g, state->decoder_tables, NaClInstStateInst(prev_inst));
          NaClExpVectorPrint(g, NaClInstStateExpVector(prev_inst));
        });
      if (NaClAssignsRegisterWithZeroExtends(
              prev_inst, NaClGet32For64BitReg(index_reg))) {
        DEBUG(NaClLog(LOG_INFO, "zero extends - safe!\n"));
        NaClMarkInstructionJumpIllegal(state, inst);
        index_reg_is_good = TRUE;
      }
    }
    if (!index_reg_is_good) {
      if (print_messages) {
        NaClValidatorInstMessage(LOG_ERROR, state, inst,
                                 "Invalid index register in memory offset\n");
      }
      return FALSE;
    }
  }
  scale_index = index_reg_index + NaClExpWidth(vector, index_reg_index);
  disp_index = scale_index + NaClExpWidth(vector, scale_index);
  DEBUG(NaClLog(LOG_INFO, "disp index = %d\n", disp_index));
  if (ExprConstant != vector->node[disp_index].kind) {
    if ((base_reg != RegRIP) ||
        (ExprConstant64 == vector->node[disp_index].kind)) {
      if (print_messages) {
        NaClValidatorInstMessage(LOG_ERROR, state, inst,
                                 "Invalid displacement in memory offset\n");
      }
      return FALSE;
    }
  }
  return TRUE;
}

void NaClMemoryReferenceValidator(NaClValidatorState* state,
                                  NaClInstIter* iter,
                                  void* ignore) {
  uint32_t i;
  NaClInstState* inst_state = state->cur_inst_state;
  NaClExpVector* vector = state->cur_inst_vector;

  DEBUG_OR_ERASE({
      struct Gio* g = NaClLogGetGio();
      NaClLog(LOG_INFO, "-> Validating store\n");
      NaClInstStateInstPrint(g, inst_state);
      NaClInstPrint(g, state->decoder_tables, state->cur_inst);
      NaClExpVectorPrint(g, vector);
    });

  /* Look for assignments on a memory offset. */
  for (i = 0; i < vector->number_expr_nodes; ++i) {
    NaClExp* node = &vector->node[i];
    if (state->quit) break;
    DEBUG(NaClLog(LOG_INFO, "processing argument %"NACL_PRIu32"\n", i));
    if (IsPossibleSandboxingNode(node)) {
      DEBUG(NaClLog(LOG_INFO, "found possible sandboxing reference\n"));
      if (NaClIsValidMemOffset(state, iter, 0, inst_state, vector, i, TRUE)) {
        continue;
      } else if (ExprSegmentAddress == node->kind) {
        /* Note that operations like stosb, stosw, stosd, and stoq use
         * segment notation. In 64 bit mode, the second argument must
         * be a register, and be computed (using lea) so that it matches
         * a valid (sandboxed) memory offset. For example:
         *
         *    mov %edi, %edi           ; zero out top half of rdi
         *    lea %rdi, (%r15, %edi, 1); calculate address, put in rdi
         *    stos %eax                ; implicity uses %es:(%rdi)
         *
         * Note: we actually allow any zero-extending operation, not just
         * an identity MOV.
         */
        int seg_prefix_reg_index;
        NaClOpKind seg_prefix_reg;
        DEBUG(NaClLog(LOG_INFO,
                      "found segment assign at node %"NACL_PRIu32"\n", i));

        /* Only allow if 64 bit segment addresses. */
        if (NACL_EMPTY_EFLAGS == (node->flags & NACL_EFLAG(ExprSize64))) {
          NaClValidatorInstMessage(
              LOG_ERROR, state, inst_state,
              "Assignment to non-64 bit segment address\n");
          continue;
        }

        /* Only allow segment prefix registers that are treated as
         * null prefixes.
         */
        seg_prefix_reg_index = NaClGetExpKidIndex(vector, i, 0);
        seg_prefix_reg = NaClGetExpVectorRegister(vector, seg_prefix_reg_index);
        switch (seg_prefix_reg) {
          case RegCS:
          case RegDS:
          case RegES:
          case RegSS:
            /* Check that we match lea constraints. */
            if (NaClInstIterHasLookbackStateInline(iter, 1)) {
              NaClInstState* prev_inst_state =
                  NaClInstIterGetLookbackStateInline(iter, 1);
              const NaClInst* prev_inst = NaClInstStateInst(prev_inst_state);
              DEBUG(NaClLog(LOG_INFO, "look at previous\n"));
              if (InstLea == prev_inst->name) {
                int seg_reg_index = NaClGetExpKidIndex(vector, i, 1);
                NaClOpKind seg_reg =
                    NaClGetExpVectorRegister(vector, seg_reg_index);
                DEBUG(NaClLog(LOG_INFO,
                              "seg reg = %s\n", NaClOpKindName(seg_reg)));
                if (seg_reg_index < (int) vector->number_expr_nodes) {
                  NaClExpVector* prev_vector =
                      NaClInstStateExpVector(prev_inst_state);
                  /* Note that first argument of LEA is always a register,
                   * (under an OperandReference), and hence is always at
                   * index 1.
                   */
                  NaClOpKind lea_reg = NaClGetExpVectorRegister(prev_vector, 1);
                  DEBUG(NaClLog(LOG_INFO,
                                "lea reg = %s\n", NaClOpKindName(lea_reg)));
                  if (lea_reg == seg_reg) {
                    /* Move to first argument which should be a memory
                     * address.
                     */
                    int memoff_index =
                        NaClGetNthExpKind(prev_vector, OperandReference, 2);
                    if (memoff_index < (int) prev_vector->number_expr_nodes) {
                      memoff_index =
                          NaClGetExpKidIndex(prev_vector, memoff_index, 0);
                      if (memoff_index < (int) prev_vector->number_expr_nodes) {
                        DEBUG(NaClLog(LOG_INFO, "check mem offset!\n"));
                        if (NaClIsValidMemOffset(state,
                                                 iter,
                                                 1,
                                                 prev_inst_state,
                                                 prev_vector,
                                                 memoff_index,
                                                 FALSE)) {
                          NaClMarkInstructionJumpIllegal(state, inst_state);
                          continue;
                        }
                      }
                    }
                  }
                }
              }
            }
            break;
          default:
            break;
        }
        /* If reached, we don't know how to handle the segment assign. */
        NaClValidatorInstMessage(LOG_ERROR, state, inst_state,
                                 "Segment memory reference not allowed\n");
      } else if (UndefinedExp == node->kind ||
                 (ExprRegister == node->kind &&
                  RegUnknown == NaClGetExpRegisterInline(node))) {
        /* First rule out case where the index registers of the memory
         * offset may be unknown.
         */
        int parent_index = NaClGetExpParentIndex(vector, i);
        if (parent_index >= 0 &&
            (i == NaClGetExpKidIndex(vector, parent_index, 1))) {
          /* Special case of memory offsets that we allow. That is, memory
           * offsets can optionally define index register. If the index
           * register isn't specified, the value RegUnknown is used as
           * a placeholder (and hence legal).
           */
        } else {
          /* This shouldn't happpen, but if it does, its because either:
           * (1) We couldn't translate the expression, and hence complain; or
           * (2) It is an X87 instruction with a register address, which we
           *     don't allow (in case these instructions get generalized in
           *     the future).
           */
          NaClValidatorInstMessage(
              LOG_ERROR, state, inst_state,
              "Memory reference not understood, can't verify correctness.\n");
        }
      }
    }
  }
  DEBUG(NaClLog(LOG_INFO, "<- Validating store\n"));
}
