/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Checks that CPU ID features match instructions found in executable.
 */

#include <stdlib.h>
#include <string.h>

#include "src/validator/x86/ncval_reg_sfi/nc_cpu_checks.h"

#include "src/platform/nacl_log.h"
#include "src/validator/x86/decoder/nc_inst_state.h"
#include "src/validator/x86/decoder/nc_inst_state_internal.h"
#include "src/validator/x86/ncval_reg_sfi/ncvalidate_iter.h"
#include "src/validator/x86/ncval_reg_sfi/ncvalidate_iter_internal.h"
#include "src/validator/x86/nc_segment.h"

#include "src/validator/x86/decoder/nc_inst_iter_inl.c"

void NaClCpuCheckMemoryInitialize(NaClValidatorState* state) {
  NaClClearCPUFeatures(&state->cpu_checks.cpu_features);
  state->cpu_checks.f_CMOV_and_x87 = FALSE;
  state->cpu_checks.f_MMX_or_SSE2 = FALSE;
}

/* Helper function to report unsupported features */
static INLINE void NaClCheckFeature(NaClCPUFeatureID feature,
                                    struct NaClValidatorState* vstate,
                                    Bool* squash_me) {
  if (!NaClGetCPUFeature(&vstate->cpu_features, feature)) {
    if (!NaClGetCPUFeature(&vstate->cpu_checks.cpu_features, feature)) {
      NaClValidatorInstMessage(
          LOG_WARNING, vstate, vstate->cur_inst_state,
          "CPU model does not support %s instructions.\n",
          NaClGetCPUFeatureName(feature));
      NaClSetCPUFeature(&vstate->cpu_checks.cpu_features, feature, TRUE);
    }
    *squash_me = TRUE;
  }
}

void NaClCpuCheck(struct NaClValidatorState* state,
                  struct NaClInstIter* iter) {
  Bool squash_me = FALSE;
  switch (state->cur_inst->insttype) {
    case NACLi_X87:
    case NACLi_X87_FSINCOS:
      NaClCheckFeature(NaClCPUFeature_x87, state, &squash_me);
      break;
    case NACLi_SFENCE_CLFLUSH:
      /* TODO(bradchen): distinguish between SFENCE and CLFLUSH */
      NaClCheckFeature(NaClCPUFeature_CLFLUSH, state, &squash_me);
      NaClCheckFeature(NaClCPUFeature_FXSR, state, &squash_me);
      break;
    case NACLi_CMPXCHG8B:
      NaClCheckFeature(NaClCPUFeature_CX8, state, &squash_me);
      break;
    case NACLi_CMPXCHG16B:
      NaClCheckFeature(NaClCPUFeature_CX16, state, &squash_me);
      break;
    case NACLi_CMOV:
      NaClCheckFeature(NaClCPUFeature_CMOV, state, &squash_me);
      break;
    case NACLi_FCMOV:
      if (!(NaClGetCPUFeature(&state->cpu_features, NaClCPUFeature_CMOV) &&
            NaClGetCPUFeature(&state->cpu_features, NaClCPUFeature_x87))) {
        if (!state->cpu_checks.f_CMOV_and_x87) {
          NaClValidatorInstMessage(
              LOG_WARNING, state, state->cur_inst_state,
              "CPU model does not support CMOV and x87 instructions.\n");
          state->cpu_checks.f_CMOV_and_x87 = TRUE;
        }
        squash_me = TRUE;
      }
      break;
    case NACLi_RDTSC:
      NaClCheckFeature(NaClCPUFeature_TSC, state, &squash_me);
      break;
    case NACLi_MMX:
      NaClCheckFeature(NaClCPUFeature_MMX, state, &squash_me);
      break;
    case NACLi_MMXSSE2:
      /* Note: We accept these instructions if either MMX or SSE2 bits */
      /* are set, in case MMX instructions go away someday...          */
      if (!(NaClGetCPUFeature(&state->cpu_features, NaClCPUFeature_MMX) ||
            NaClGetCPUFeature(&state->cpu_features, NaClCPUFeature_SSE2))) {
        if (!state->cpu_checks.f_MMX_or_SSE2) {
          NaClValidatorInstMessage(
              LOG_WARNING, state, state->cur_inst_state,
              "CPU model does not support MMX or SSE2 instructions.\n");
          state->cpu_checks.f_MMX_or_SSE2 = TRUE;
        }
      }
      squash_me = TRUE;
      break;
    case NACLi_SSE:
      NaClCheckFeature(NaClCPUFeature_SSE, state, &squash_me);
      break;
    case NACLi_SSE2:
      NaClCheckFeature(NaClCPUFeature_SSE2, state, &squash_me);
      break;
    case NACLi_SSE3:
      NaClCheckFeature(NaClCPUFeature_SSE3, state, &squash_me);
      break;
    case NACLi_SSE4A:
      NaClCheckFeature(NaClCPUFeature_SSE4A, state, &squash_me);
      break;
    case NACLi_SSE41:
      NaClCheckFeature(NaClCPUFeature_SSE41, state, &squash_me);
      break;
    case NACLi_SSE42:
      NaClCheckFeature(NaClCPUFeature_SSE42, state, &squash_me);
      break;
    case NACLi_MOVBE:
      NaClCheckFeature(NaClCPUFeature_MOVBE, state, &squash_me);
      break;
    case NACLi_POPCNT:
      NaClCheckFeature(NaClCPUFeature_POPCNT, state, &squash_me);
      break;
    case NACLi_LZCNT:
      NaClCheckFeature(NaClCPUFeature_LZCNT, state, &squash_me);
      break;
    case NACLi_SSSE3:
      NaClCheckFeature(NaClCPUFeature_SSSE3, state, &squash_me);
      break;
    case NACLi_3DNOW:
      NaClCheckFeature(NaClCPUFeature_3DNOW, state, &squash_me);
      break;
    case NACLi_E3DNOW:
      NaClCheckFeature(NaClCPUFeature_E3DNOW, state, &squash_me);
      break;
    case NACLi_LONGMODE:
      /* TODO(karl): Remove this when NACLi_LONGMODE is no longer needed */
      NaClCheckFeature(NaClCPUFeature_LM, state, &squash_me);
      break;
    case NACLi_SSE2x:
      /* This case requires CPUID checking code */
      /* DATA16 prefix required */
      if (!(state->cur_inst_state->prefix_mask & kPrefixDATA16)) {
        NaClValidatorInstMessage(
            LOG_ERROR, state, state->cur_inst_state,
            "SSEx instruction must use prefix 0x66.\n");
      }
      NaClCheckFeature(NaClCPUFeature_SSE2, state, &squash_me);
      break;
    default:
      /* This instruction could be either legal or illegal, but if we
       * get here it is not CPU-dependent.
       */
      break;
  }
  if (state->cur_inst->flags & NACL_IFLAG(LongMode)) {
    NaClCheckFeature(NaClCPUFeature_LM, state, &squash_me);
  }
  if (squash_me) {
    if (state->readonly_text) {
      NaClValidatorInstMessage(
          LOG_ERROR, state, state->cur_inst_state,
          "Read-only text: cannot squash unsupported instruction.\n");
    } else {
      /* Replace all bytes of the instruction with the HLT instruction. */
      NCStubOutMem(state, NaClInstIterGetInstMemoryInline(iter),
                   NaClInstStateLength(state->cur_inst_state));
    }
  }
}
