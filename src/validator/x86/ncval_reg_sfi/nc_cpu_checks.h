/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Checks that CPU ID features match instructions found in executable.
 *
 * Note: The following functions are used to define a validator function
 * for collecting this information. See header file ncvalidator_iter.h
 * for more information on how to register these functions as a validator
 * function.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_CPU_CHECKS_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_CPU_CHECKS_H__

#include "src/utils/types.h"
#include "src/validator/x86/nacl_cpuid.h"

/* Defines a validator state. */
struct NaClValidatorState;

/* Defines an instruction iterator that processes a code segment. */
struct NaClInstIter;

/* Defines the set of cpu feature checks applied to the code. */
typedef struct NaClCpuCheckState {
  /* The standard CPU features. */
  NaClCPUFeaturesX86 cpu_features;
  /* Check that both f_CMOV and f_x87 is defined. */
  Bool f_CMOV_and_x87;
  /* Check that either f_MMX or f_SSE2 is defined. */
  Bool f_MMX_or_SSE2;
} NaClCpuCheckState;

/* Creates a CPU feature struct (all fields initialized to false), to be used
 * to record what features need to be squashed out of the executable.
 */
void NaClCpuCheckMemoryInitialize(struct NaClValidatorState* state);

/* Check that cpu features match instructions for native client rules. */
void NaClCpuCheck(struct NaClValidatorState* state,
                  struct NaClInstIter* iter);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_CPU_CHECKS_H__ */
