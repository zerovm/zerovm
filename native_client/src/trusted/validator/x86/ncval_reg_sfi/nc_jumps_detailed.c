/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * nc_jumps_detailed.c - Validate jumps using a second pass of the
 * isntructions.
 */

#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_jumps_detailed.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_state_internal.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter_internal.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_jumps.h"

#include "native_client/src/trusted/validator/x86/decoder/nc_inst_iter_inl.c"

/* Returns true if the given address is within the code segment.
 */
static Bool NaClCheckAddressRange(NaClPcAddress address,
                                  NaClValidatorState* state) {
  if (address < state->vbase) {
    return FALSE;
  }
  if (address >= state->vlimit) {
    return FALSE;
  }
  return TRUE;
}

static void NaClInstLayoutCheck(NaClValidatorState* state,
                                NaClInstState* inst_state,
                                NaClJumpSets* jump_sets) {
  NaClPcAddress start;
  NaClPcAddress end;
  NaClPcAddress i;
  if (NULL == inst_state) return;

  /* Check basic block boundaries. */
  start = NaClInstStateVpc(inst_state);

  /* Check that if first instruction in a basic block, it isn't in the
   * middle of a pattern.
   */
  if ((0 == (start % state->alignment)) &&
      NaClAddressSetContains(jump_sets->removed_targets, start, state)) {
    NaClValidatorInstMessage(
        LOG_ERROR, state, inst_state,
        "Instruction begins basic block, but in middle of nacl pattern\n");
  }

  /* Check that instruction doesn't cross block boundaries. */
  end = (NaClPcAddress) (start + inst_state->bytes.length);
  for (i = start + 1; i < end; ++i) {
    if (0 == (i % state->alignment)) {
      NaClValidatorInstMessage(
          LOG_ERROR, state, inst_state,
          "Instruction crosses basic block alignment\n");
    }
  }

  /* Check jump targets. */
  if (NaClHasBit(inst_state->inst->flags,
                 NACL_IFLAG(JumpInstruction) | NACL_IFLAG(ConditionalJump))) {
    uint32_t i;
    NaClExpVector* vector = NaClInstStateExpVector(inst_state);
    for (i = 0; i < vector->number_expr_nodes; ++i) {
      if (NaClHasBit(vector->node[i].flags, NACL_EFLAG(ExprJumpTarget))) {
        switch (vector->node[i].kind) {
          case ExprConstant:
          case ExprConstant64:
            {
              /* Explicit jump value. Check if legal! */
              NaClPcAddress target =
                  (NaClPcAddress) NaClGetExpConstant(vector, i);
              /* Don't report targets that are out of range. They should have
               * been reported in the first pass!
               */
              if (NaClCheckAddressRange(target, state)) {
                if (NaClAddressSetContains(jump_sets->possible_targets,
                                           target, state)) {
                  if (NaClAddressSetContains(jump_sets->removed_targets,
                                             target, state)) {
                    NaClValidatorInstMessage(
                        LOG_ERROR, state, inst_state,
                        "Jumps into middle of nacl pattern\n");
                  }
                } else {
                  NaClValidatorInstMessage(
                      LOG_ERROR, state, inst_state,
                      "Doesn't jump to instruction address\n");
                }
              }
            }
            break;
          default:
            break;
        }
      }
    }
  }
}

void NaClJumpValidatorSummarizeDetailed(NaClValidatorState* state,
                                        NaClInstIter* iter,
                                        NaClJumpSets* jump_sets) {
  NaClInstIter *check_iter;
  if (state->quit) return;
  NaClValidatorMessage(
      LOG_INFO, state,
      "Checking block alignment and jump targets: %"NACL_PRIxNaClPcAddress
      " to %"NACL_PRIxNaClPcAddress"\n",
      state->vbase, state->vlimit);

  /* Check that code segment starts at an aligned address. */
  if (state->vbase & state->alignment_mask) {
    NaClValidatorMessage(
        LOG_ERROR, state,
        "Code segment starts at 0x%"NACL_PRIxNaClPcAddress", "
        "which isn't aligned properly.\n",
        state->vbase);
  }

  /* Perform second pass on instructions to find instructions crossing
   * block boundaries, and (explicitly) branch to bad places.
   */
  do {
    check_iter = NaClInstIterCreate(state->decoder_tables, iter->segment);
    if (NULL == check_iter) {
      NaClValidatorMessage(LOG_ERROR, state,
                           "Not enough memory to check jumps\n");
      break;
    }
    for (; NaClInstIterHasNextInline(check_iter);
         NaClInstIterAdvanceInline(check_iter)) {
      NaClInstState* inst_state = NaClInstIterGetStateInline(check_iter);
      NaClInstLayoutCheck(state, inst_state, jump_sets);
    }
    NaClInstIterDestroy(check_iter);
  } while (0);
}
