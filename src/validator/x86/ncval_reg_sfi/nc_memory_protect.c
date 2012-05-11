/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/validator/x86/ncval_reg_sfi/nc_memory_protect.h"

#include "include/portability_io.h"
#include "src/platform/nacl_log.h"
#include "src/validator/x86/decoder/nc_inst_state.h"
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

#include "src/validator/x86/decoder/nc_inst_iter_inl.c"
#include "src/validator/x86/decoder/ncop_exps_inl.c"

/* Maximum character buffer size to use for generating messages. */
static const size_t kMaxBufferSize = 1024;

/*
 * When true, check both uses and sets of memory. When false, only
 * check sets.
 */
Bool NACL_FLAGS_read_sandbox = TRUE;

/* Type used to measure number of instructions matched using a validator
 * pattern.
 */
typedef int NaClPatternLength;

/* Constant denoting that pattern match failed. Used by pattern checking
 * functions that return the number of instructions pattern matched.
 */
static const NaClPatternLength kNaClPatternMatchFailed = -1;

/* Returns the instruction "distance" elements back from the current
 * instruction of the validator state. Returns NULL if no such instruction
 * exists.
 */
static INLINE NaClInstState* NaClGetValInstStateAt(NaClValidatorState* state,
                                                   size_t distance) {
  if (distance == 0) {
    return state->cur_inst_state;
  } else if (NaClInstIterHasLookbackStateInline(state->cur_iter, distance)) {
    return NaClInstIterGetLookbackStateInline(state->cur_iter, distance);
  } else {
    return NULL;
  }
}

/* Returns true if the given register is in {%RSP, %RBP, %RIP, %RBASE}.*/
static INLINE Bool NaClIsValidBaseRegister(NaClValidatorState* state,
                                           NaClOpKind reg) {
  return  reg == state->base_register ||
      reg == RegRSP ||
      reg == RegRBP ||
      reg == RegRIP;
}

/* Returns true if the node corresponds to an expression set, or an
 * expression use if we are doing read sandboxing.
 */
static Bool IsPossibleSandboxingNode(NaClExp* node) {
  return ((NACL_FLAGS_read_sandbox && (node->flags & NACL_EFLAG(ExprUsed))) ||
          (node->flags & NACL_EFLAG(ExprSet)));
}

/* Returns true if the "distance" instruction, from the current instruction
 * of the validator state is a mov of the form:
 *    mov %reg, %reg
 * and %reg is a 32-bit register.
 */
static Bool NaClIsMov32UsingReg(NaClValidatorState* state,
                                size_t distance,
                                NaClOpKind reg) {
  NaClExpVector* vector;
  NaClInstState* inst_state;

#ifdef NCVAL_TESTING
  /* Assume we match previous instructions when generating pre/post
   * conditions.
   */
  if (distance > 0) return TRUE;
#endif

  /* Get the instruction to be checked. */
  inst_state = NaClGetValInstStateAt(state, distance);
  if (NULL == inst_state) return FALSE;

  /* Check that it is a move on the specified register. */
  vector = NaClInstStateExpVector(inst_state);
  if (!NaClIsMovUsingRegisters(inst_state->decoder_tables,
                               inst_state->inst,
                               vector,
                               reg,
                               reg)) return FALSE;

  /* Check that this is a 32-bit mov.
   * Note: Since the vector contains a list of operand expressions, the
   * first operand reference is always at index zero, and its first child
   * (where the register set is defined) is at index 1.
   */
  return NaClHasBit(vector->node[1].flags, NACL_EFLAG(ExprSize32));
}

/* Checks if the given node index for the given instruction is
 * a valid (sandboxed) memory offset, based on NACL rules, returning TRUE iff
 * the memory offset is NACL compliant. That is, of one of
 * the following forms:
 * [%base+%r64*n+d32]
 *    where the previous instruction is of the form:
 *    op %r32, ..               ; zero out top half of r32
 *    and r32 is the corresponding 32-bit register for the 64-bit register
 *    r64
 * [%base+d32]
 * [%base]
 *
 * where rbase is in { RIP, RSP, RBP, RZP }, and d32 must not exceed 32 bits.
 *
 * parameters are:
 *    state - The state of the validator,
 *    distance - number of instructions to lookback
 *      (within the iterator) in order to retrieve the instruction.
 *    node_index - The index of the memory offset within the given
 *      instruction vector.
 *    use_mov_for_zero_ext - True if zero extending op must be of the
 *      form "mov %r32, %r32".
 *    print_messages - True if this routine is responsable for printing
 *      error messages if the memory offset isn't NACL compliant.
 *
 * Returns the number of (additional) instructions were needed to recognize
 * the pattern, or kNaClPatternMatchFailed if the pattern fails.
 */
static NaClPatternLength NaClMatchValidMemOffset(
    NaClValidatorState* state,
    size_t distance,
    int node_index,
    Bool use_mov_for_zero_ext,
    Bool print_messages) {
  int base_reg_index;
  NaClOpKind base_reg;
  int index_reg_index;
  NaClExp* index_reg_node;
  NaClOpKind index_reg;
  int scale_index;
  int disp_index;
  NaClInstState* inst;
  NaClExpVector* vector;
  NaClExp* node;
  NaClPatternLength pattern_length = 0;

#ifdef NCVAL_TESTING
  /* Assume we  match previous instructions when generating pre/post
   * conditions.
   */
  if (distance > 0) return pattern_length;
#endif

  /* Get the instruction to be checked. */
  inst = NaClGetValInstStateAt(state, distance);
  if (NULL == inst) return kNaClPatternMatchFailed;

  /* Check that the node_index of the referenced instruction corresponds to
   * a memory offset.
   */
  vector = NaClInstStateExpVector(inst);
  node = &vector->node[node_index];
  if (ExprMemOffset != node->kind) return kNaClPatternMatchFailed;

  DEBUG(NaClLog(LOG_INFO,
                "found MemOffset at node %"NACL_PRIu32" %d\n", node_index,
                (int) distance));

  /* Only allow memory offset nodes with address size 64. */
  if (NACL_EMPTY_EFLAGS == (node->flags & NACL_EFLAG(ExprSize64))) {
    if (print_messages) {
      NaClValidatorInstMessage(LOG_ERROR, state, inst,
                               "Assignment to non-64 bit memory address\n");
    }
    return kNaClPatternMatchFailed;
  }
  DEBUG(NaClLog(LOG_INFO, "found 64 bit address for MemOffset\n"));

  /* Check that the base register is valid. */
  base_reg_index = node_index + 1;
  base_reg = NaClGetExpVectorRegister(vector, base_reg_index);
  DEBUG(NaClLog(LOG_INFO, "base reg = %s\n", NaClOpKindName(base_reg)));
  if (!NaClIsValidBaseRegister(state, base_reg)) {
    if (print_messages) {
      NaClValidatorInstMessage(
          LOG_ERROR, state, inst,
          (base_reg == RegUnknown
           ? "No base register specified in memory offset\n"
           : "Invalid base register in memory offset\n"));
    }
    return kNaClPatternMatchFailed;
  }
  DEBUG(NaClLog(LOG_INFO, "  => base register is valid\n"));

  /* Check that the index register is either undefined, or defined
   * with the appropriate assignment. Note: We currently don't allow
   * an index register to be used with RIP. Only "RIP + disp32" is allowed.
   */
  index_reg_index = base_reg_index + NaClExpWidth(vector, base_reg_index);
  index_reg_node = &vector->node[index_reg_index];
  index_reg = NaClGetExpRegisterInline(index_reg_node);
  DEBUG(NaClLog(LOG_INFO, "index reg = %s\n", NaClOpKindName(index_reg)));
  if (RegUnknown != index_reg) {
    Bool index_reg_is_good = FALSE;
    if ((base_reg != RegRIP) &&
        NaClHasBit(index_reg_node->flags, NACL_EFLAG(ExprSize64))) {
      if (use_mov_for_zero_ext) {
        index_reg_is_good =
            NaClIsMov32UsingReg(state, distance + 1,
                                NaClGet32For64BitReg(index_reg));
      } else {
        index_reg_is_good =
            NaClAssignsRegisterWithZeroExtends64(state, distance + 1,
                                                 index_reg);
      }
    }
    if (index_reg_is_good) {
      pattern_length++;
    } else {
      if (print_messages) {
        NaClValidatorInstMessage(LOG_ERROR, state, inst,
                                 "Invalid index register in memory offset\n");
      }
      return kNaClPatternMatchFailed;
    }
  } else if (use_mov_for_zero_ext) {
    /* The index register must be defined in this case. Report as error!. */
    if (print_messages) {
      NaClValidatorInstMessage(LOG_ERROR, state, inst,
                               "Invalid index register in memory offset\n");
      return kNaClPatternMatchFailed;
    }
  }

  /* Check displacement value is valid (i.e. 32 or less bits).
   * Note: scale need not be
   * checked since it can't exceed 8, and is undefined if the index register
   * is undefined.
   */
  scale_index = index_reg_index + NaClExpWidth(vector, index_reg_index);
  disp_index = scale_index + NaClExpWidth(vector, scale_index);
  DEBUG(NaClLog(LOG_INFO, "disp index = %d\n", disp_index));
  if (ExprConstant != vector->node[disp_index].kind) {
    if (print_messages) {
      NaClValidatorInstMessage(LOG_ERROR, state, inst,
                               "Invalid displacement in memory offset\n");
    }
    return kNaClPatternMatchFailed;
  }

  /* If reached, we matched the pattern, return number of instructions
   * needed for match.
   */
  DEBUG(NaClLog(LOG_INFO, "Memory Offset pattern length = %d\n",
                pattern_length));
#ifdef NCVAL_TESTING
  if ((0 == distance) && (RegUnknown != index_reg)) {
    /* Report precondition of test.
     * TODO(karl): This should be lifted out of this test, and
     * only applied when pattern matches.
     */
    char* buffer;
    size_t buffer_size;
    char reg_name[kMaxBufferSize];
    NaClOpRegName(NaClGet32For64BitReg(index_reg), reg_name, kMaxBufferSize);
    NaClConditionAppend(state->precond, &buffer, &buffer_size);
    SNPRINTF(buffer, buffer_size, "ZeroExtends(%s)", reg_name);
  }
#endif
  return pattern_length;
}

/* Applies the precondition "SafeAddress(addr_reg)" pattern for
 * segment addresses. Applies the match to the "distance" instruction
 * back from the current instruction. Returns true if the instruction
 * is a pair of instructions of the form:
 *
 *    mov %r32, %r32            ; zero out top half of r32
 *    lea %r64, [%r15+%r64*1]   ; rebase address and put in r64
 *
 * where r32 is the corresponding 32-bit register for the 64-bit register
 * r64. Returns true if the precondition is met.
 */
static Bool NaClMatchLeaSafeAddress(
    NaClValidatorState* state,
    size_t distance,
    NaClOpKind reg64) {
  NaClExpVector* vector;
  NaClOpKind lea_reg;
  int memoff_index;
  NaClPatternLength pattern_length;
  NaClInstState* inst_state;
  const NaClInst* inst;

  DEBUG(NaClLog(LOG_INFO, "reg64 = %s, distance = %d\n",
                NaClOpKindName(reg64), (int) distance));

#ifdef NCVAL_TESTING
  /* Don't match previous instructions when pattern matching. */
  if (distance > 0) return TRUE;
#endif

  /* Get the instruction to be checked. */
  inst_state = NaClGetValInstStateAt(state, distance);
  if (NULL == inst_state) return FALSE;

  /* Check that it is an LEA instruction. */
  inst = NaClInstStateInst(inst_state);
  if (InstLea != inst->name) return FALSE;

  /* Note that first argument of LEA is always a register,
   * (under an OperandReference), and hence is always at
   * index 1.
   */
  vector = NaClInstStateExpVector(inst_state);
  lea_reg = NaClGetExpVectorRegister(vector, 1);
  DEBUG(NaClLog(LOG_INFO, "lea reg = %s\n", NaClOpKindName(lea_reg)));
  if (lea_reg != reg64) return FALSE;

  /* Move to second argument which should be a memory address. */
  memoff_index = NaClGetNthExpKind(vector, OperandReference, 2);
  if (-1 == memoff_index) return FALSE;
  memoff_index = NaClGetExpKidIndex(vector, memoff_index, 0);
  if (memoff_index >= (int) vector->number_expr_nodes) return FALSE;
  DEBUG(NaClLog(LOG_INFO, "check mem offset!\n"));

  /* Check that memory offset argument of the lea is valid. */
  pattern_length = NaClMatchValidMemOffset(state,
                                           distance,
                                           memoff_index,
                                           TRUE,
                                           FALSE);
  if (kNaClPatternMatchFailed == pattern_length) return FALSE;

  /* If reached, matched pattern! */
  return TRUE;
}

void NaClMemoryReferenceValidator(NaClValidatorState* state) {
  /* Note: This code assumes that only DS:RSI and ES:RDI allow
   * multiple memory accesses with a single instruction, as defined
   * by the string instructions Movs and Cmps. In such cases, the
   * ES:RDI must appear before the DS:RSI.
   */
  int i;
  NaClInstState* inst_state = state->cur_inst_state;
  NaClExpVector* vector = state->cur_inst_vector;

  /* Holds the number of additional instructions in pattern(s) associated with
   * the current instruction of the validator state.
   */
  int pattern_length = 0;

  /* Counts number of memory references in the current instruction. */
  int number_memory_refs = 0;

  /* Counts the number of unique segment (memory) references in the current
   * instruction.
   */
  int number_segment_addresses = 0;

  /* Records first segment reference register, to allow duplication of it.
   */
  NaClOpKind previous_seg_addr = RegUnknown;

  DEBUG_OR_ERASE({
      struct Gio* g = NaClLogGetGio();
      NaClLog(LOG_INFO, "-> Validating store\n");
      NaClInstStateInstPrint(g, inst_state);
      NaClInstPrint(g, state->decoder_tables, state->cur_inst);
      NaClExpVectorPrint(g, inst_state);
    });

  /* Look for assignments on a memory offset. */
  for (i = 0; i < (int) vector->number_expr_nodes; ++i) {
    /* Note: continue (within this loop) is used to indicate
     * that the node describes a safe memory address.
     */
    NaClPatternLength memoffset_length;
    NaClExp* node = &vector->node[i];
    DEBUG(NaClLog(LOG_INFO, "pattern length = %d\n", pattern_length);
          NaClLog(LOG_INFO, "processing argument %"NACL_PRIu32"\n", i));
    if (state->quit) break;

    if (!IsPossibleSandboxingNode(node)) continue;
    DEBUG(NaClLog(LOG_INFO, "found possible sandboxing reference\n"));

    /* Check if the node is a safe memory offset (i.e address). */
    memoffset_length = NaClMatchValidMemOffset(state, 0, i, FALSE, TRUE);
    DEBUG(NaClLog(LOG_INFO, "memoffset inst length = %d\n", memoffset_length));
    if (kNaClPatternMatchFailed != memoffset_length) {
      /* Cases 1, 2, or 3 (see nc_memory_protect.h). */
      number_memory_refs++;
      pattern_length += memoffset_length;
      continue;
    }

    if (ExprSegmentAddress == node->kind) {
      /* Note that operations like stosb, stosw, stosd, and stosq use
       * segment notation. In 64 bit mode, the second argument must
       * be a register, and be computed (using lea) so that it matches
       * a valid (sandboxed) memory offset. For example:
       *
       *    mov %edi, %edi           ; zero out top half of rdi
       *    lea %rdi, [%r15+%edi*1]  ; calculate address, put in rdi
       *    stos %eax                ; implicity uses %es:(%rdi)
       *
       * Note: we allow only one zero-extending operation for string
       * instructions: an identity MOV.
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
        /* Note: continue (of the for loop) is used to indicate a good segment
         * address (for node being checked) while break (of the switch) is
         * used to indicate a bad segment address.
         */
        case RegCS:
        case RegDS:
        case RegES:
        case RegSS: {
          NaClOpKind addr_reg;

          /* Get the address register of the segment memory address. */
          int addr_reg_index = NaClGetExpKidIndex(vector, i, 1);
          DEBUG(NaClLog(LOG_INFO,
                        "matched segment %s, address at index %d\n",
                        NaClOpKindName(seg_prefix_reg), addr_reg_index));
          if (-1 == addr_reg_index) break;
          addr_reg =
              NaClGetExpVectorRegister(vector, addr_reg_index);
          if (addr_reg == RegUnknown) break;

          /* If addr_reg a previously known register, allow. */
          if (previous_seg_addr == RegUnknown) {
            previous_seg_addr = addr_reg;
          } else if (addr_reg == previous_seg_addr) {
            /* No need to check new pattern, existing pattern
             * should check.
             */
            continue;
          }

          /* Check that the address register is an lea address
           * that is safe. */
          if (!NaClMatchLeaSafeAddress(state, pattern_length + 1,
                                       addr_reg)) break;

          /* Case 4 (see nc_memory_protect.h). */
          number_memory_refs++;
          number_segment_addresses++;
          pattern_length += 2;
          DEBUG(NaClLog(LOG_INFO, "updated pattern_length = %d\n",
                        pattern_length));
#ifdef NCVAL_TESTING
          {
            /* Assume that previous instruction is an LEA, since
             * we want to only generate pre/post conditions.
             */
            char* buffer;
            size_t buffer_size;
            char reg_name[kMaxBufferSize];
            NaClOpRegName(addr_reg, reg_name, kMaxBufferSize);
            NaClConditionAppend(state->precond, &buffer, &buffer_size);
            SNPRINTF(buffer, buffer_size, "SafeAddress(%s)", reg_name);
          }
#endif
          continue;
        }
        default:
          break;
      }

      /* If reached, we don't know how to handle the segment address. */
      NaClValidatorInstMessage(LOG_ERROR, state, inst_state,
                                 "Segment memory reference not allowed\n");
      continue;
    }

    /* Don't complain about register set/usage. */
    if (UndefinedExp == node->kind ||
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

  /* Disallow multiple memory references unless pair of special segment refs. */
  if ((number_memory_refs > 1) &&
      (number_segment_addresses != number_memory_refs)) {
    NaClValidatorInstMessage(
        LOG_ERROR, state, inst_state,
        "Multiple memory references not allowed in this context\n");
  }

#ifndef NCVAL_TESTING
  /* Mark all but first instruction of pattern illegal to jump into. */
  if (0 < pattern_length) {
    NaClMarkInstructionsJumpRangeIllegal(state, pattern_length);
  }
#endif
  DEBUG(NaClLog(LOG_INFO, "<- Validating store\n"));
}

#ifdef NCVAL_TESTING
Bool NaClAcceptLeaSafeAddress(struct NaClValidatorState* state) {
  const NaClInst* inst;

  /* Check that it is an LEA instruction. */
  inst = NaClInstStateInst(state->cur_inst_state);
  if (InstLea != inst->name) return FALSE;

  /* Note that first argument of LEA is always a register,
   * (under an OperandReference), and hence is always at
   * index 1.
   */
  return NaClMatchLeaSafeAddress(
      state, 0,
      NaClGetExpVectorRegister(NaClInstStateExpVector(state->cur_inst_state),
                               1));
}
#endif
