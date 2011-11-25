/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Checks that CPU ID features match instructions found in executable.
 */

#include <stdlib.h>
#include <string.h>

#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_cpu_checks.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/validator/x86/nacl_cpuid.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_state.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_state_internal.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter_internal.h"
#include "native_client/src/trusted/validator/x86/nc_segment.h"

#include "native_client/src/trusted/validator/x86/decoder/nc_inst_iter_inl.c"

typedef struct NaClCpuCheckState {
  /* The standard CPU features. */
  CPUFeatures cpu_features;
  /* Check that both f_CMOV and f_x87 is defined. */
  Bool f_CMOV_and_x87;
  /* Check that either f_MMX or f_SSE2 is defined. */
  Bool f_MMX_or_SSE2;
} NaClCpuCheckState;

NaClCpuCheckState* NaClCpuCheckMemoryCreate(NaClValidatorState* state) {
  NaClCpuCheckState* check_state =
      (NaClCpuCheckState*) calloc(1, sizeof(NaClCpuCheckState));
  NaClClearCPUFeatures(&check_state->cpu_features);
  check_state->f_CMOV_and_x87 = FALSE;
  check_state->f_MMX_or_SSE2 = FALSE;
  return check_state;
}

void NaClCpuCheckMemoryDestroy(NaClValidatorState* state,
                               NaClCpuCheckState* checked_features) {
  free(checked_features);
}

/* Helper macro to report unsupported features */
#define NACL_CHECK_FEATURE(feature, feature_name) \
  if (!cpu_features->feature) { \
    if (!checked_features->cpu_features.feature) { \
      NaClValidatorInstMessage( \
          LOG_WARNING, state, inst_state, \
          "Does not support " feature_name " feature, removing usage(s).\n"); \
      checked_features->cpu_features.feature = TRUE; \
    } \
    squash_me = TRUE; \
  }

void NaClCpuCheck(struct NaClValidatorState* state,
                  struct NaClInstIter* iter,
                  NaClCpuCheckState* checked_features) {
  NaClInstState* inst_state = state->cur_inst_state;
  Bool squash_me = FALSE;
  CPUFeatures *cpu_features = &state->cpu_features;
  switch (state->cur_inst->insttype) {
    case NACLi_X87:
      NACL_CHECK_FEATURE(f_x87, "x87");
      break;
    case NACLi_SFENCE_CLFLUSH:
      /* TODO(bradchen): distinguish between SFENCE and CLFLUSH */
      NACL_CHECK_FEATURE(f_CLFLUSH, "CLFLUSH");
      NACL_CHECK_FEATURE(f_FXSR, "SFENCE");
      break;
    case NACLi_CMPXCHG8B:
      NACL_CHECK_FEATURE(f_CX8, "CX8");
      break;
    case NACLi_CMPXCHG16B:
      NACL_CHECK_FEATURE(f_CX16, "CX16");
      break;
    case NACLi_CMOV:
      NACL_CHECK_FEATURE(f_CMOV, "CMOV");
      break;
    case NACLi_FCMOV:
      if (!(cpu_features->f_CMOV && cpu_features->f_x87)) {
        if (!checked_features->f_CMOV_and_x87) {
          NaClValidatorInstMessage(
              LOG_WARNING, state, inst_state,
              "Does not support CMOV and x87 features, "
              "removing corresponding CMOV usage(s).\n");
          checked_features->f_CMOV_and_x87 = TRUE;
        }
        squash_me = TRUE;
      }
      break;
    case NACLi_RDTSC:
      NACL_CHECK_FEATURE(f_TSC, "TSC");
      break;
    case NACLi_MMX:
      NACL_CHECK_FEATURE(f_MMX, "MMX");
      break;
    case NACLi_MMXSSE2:
      /* Note: We accept these instructions if either MMX or SSE2 bits */
      /* are set, in case MMX instructions go away someday...          */
      if (!(cpu_features->f_MMX || cpu_features->f_SSE2)) {
        if (!checked_features->f_MMX_or_SSE2) {
          NaClValidatorInstMessage(
              LOG_WARNING, state, inst_state,
              "Does not support MMX or SSE2 features, "
              "removing corresponding usage(s).\n");
          checked_features->f_MMX_or_SSE2 = TRUE;
        }
      }
      squash_me = TRUE;
      break;
    case NACLi_SSE:
      NACL_CHECK_FEATURE(f_SSE, "SSE");
      break;
    case NACLi_SSE2:
      NACL_CHECK_FEATURE(f_SSE2, "SSE2");
      break;
    case NACLi_SSE3:
      NACL_CHECK_FEATURE(f_SSE3, "SSE3");
      break;
    case NACLi_SSE4A:
      NACL_CHECK_FEATURE(f_SSE4A, "SSE4A");
      break;
    case NACLi_SSE41:
      NACL_CHECK_FEATURE(f_SSE41, "SSE41");
      break;
    case NACLi_SSE42:
      NACL_CHECK_FEATURE(f_SSE42, "SSE42");
      break;
    case NACLi_MOVBE:
      NACL_CHECK_FEATURE(f_MOVBE, "MOVBE");
      break;
    case NACLi_POPCNT:
      NACL_CHECK_FEATURE(f_POPCNT, "POPCNT");
      break;
    case NACLi_LZCNT:
      NACL_CHECK_FEATURE(f_LZCNT, "LZCNT");
      break;
    case NACLi_SSSE3:
      NACL_CHECK_FEATURE(f_SSSE3, "SSSE3");
      break;
    case NACLi_3DNOW:
      NACL_CHECK_FEATURE(f_3DNOW, "3DNOW");
      break;
    case NACLi_E3DNOW:
      NACL_CHECK_FEATURE(f_E3DNOW, "E3DNOW");
      break;
    case NACLi_LONGMODE:
      /* TODO(karl): Remove this when NACLi_LONGMODE is no longer needed */
      NACL_CHECK_FEATURE(f_LM, "LM");
      break;
    case NACLi_SSE2x:
      /* This case requires CPUID checking code */
      /* DATA16 prefix required */
      if (!(inst_state->prefix_mask & kPrefixDATA16)) {
        NaClValidatorInstMessage(
            LOG_ERROR, state, inst_state,
            "SSEx instruction must use prefix 0x66.\n");
      }
      NACL_CHECK_FEATURE(f_SSE2, "SSE2");
      break;
    default:
      break;
  }
  if (state->cur_inst->flags & NACL_IFLAG(LongMode)) {
    NACL_CHECK_FEATURE(f_LM, "LM");
  }
  if (squash_me) {
    /* Replace all bytes with the stop instruction. */
    memset(NaClInstIterGetInstMemoryInline(iter), kNaClFullStop,
           NaClInstStateLength(inst_state));
  }
}
