/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATE_ITER_INTERNAL_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATE_ITER_INTERNAL_H__

/* Defines the internal data structure for the validator state. */

#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/validator/x86/nacl_cpuid.h"

struct NaClDecodeTables;
struct NaClExpVector;
struct NaClInst;
struct NaClInstIter;
struct NaClInstState;
struct NaClValidatorState;

/* Defines the maximum number of validators that can be registered. */
#define NACL_MAX_NCVALIDATORS 20

/* Holds the registered definition for a validator. */
typedef struct NaClValidatorDefinition {
  /* The validator function to apply. */
  NaClValidator validator;
  /* The post iterator validator function to apply, after iterating
   * through all instructions in a segment. If non-null, called to
   * do corresponding post processing.
   */
  NaClValidatorPostValidate post_validate;
  /* The corresponding statistic print function associated with the validator
   * function (may be NULL).
   */
  NaClValidatorPrintStats print_stats;
  /* The corresponding memory creation fuction associated with the validator
   * function (may be NULL).
   */
  NaClValidatorMemoryCreate create_memory;
  /* The corresponding memory clean up function associated with the validator
   * function (may be NULL).
   */
  NaClValidatorMemoryDestroy destroy_memory;
} NaClValidatorDefinition;

/* Defines a function that implements validator summary rules. When in sel_ldr,
 * this will be the minimal code needed to detect problems. When in ncval,
 * this will expend more effort by walking the code a second time and
 * generate more readable error messages.
 */
typedef void (*NaClValidatorRulesInitFn)(struct NaClValidatorState *state);

struct NaClValidatorState {
  /* Holds the decoder tables to use. */
  const struct NaClDecodeTables* decoder_tables;
  /* Holds the vbase value passed to NaClValidatorStateCreate. */
  NaClPcAddress vbase;
  /* Holds the alignment value passed to NaClValidatorStateCreate. */
  uint8_t alignment;
  /* Holds the upper limit of all possible addresses */
  NaClPcAddress vlimit;
  /* Holds the alignment mask, which when applied, catches any lower
   * bits in an address that violate alignment.
   */
  NaClPcAddress alignment_mask;
  /* Holds the value for the base register, or RegUnknown if undefined. */
  NaClOpKind base_register;
  /* Holds if the validation is still valid. */
  Bool validates_ok;
  /* If >= 0, holds how many errors can be reported. If negative,
   * reports all errors.
   */
  int quit_after_error_count;
  /* Holds the error reporting object to use. */
  NaClErrorReporter* error_reporter;
  /* Holds the set of validators to apply. */
  NaClValidatorDefinition validators[NACL_MAX_NCVALIDATORS];
  /* Holds the local memory associated with validators to be applied to this
   * state.
   */
  void* local_memory[NACL_MAX_NCVALIDATORS];
  /* Defines how many validators are defined for this state. */
  int number_validators;
  /* Holds the cpu features of the machine it is running on. */
  CPUFeatures cpu_features;
  /* Flag controlling whether an opcode histogram is collected while
   * validating.
   */
  Bool print_opcode_histogram;
  /* Flag controling whether each in struciton is traced while validating
   * instructions.
   */
  Bool trace_instructions;
  /* Flag controlling whether the internals of each instruction is traced
   * as they are visited by the validator.
   */
  Bool trace_inst_internals;
  /* Defines the verbosity of messages to print. */
  int log_verbosity;
  /* Cached instruction state. Only guaranteed to be defined when a
   * NaClValidator is called. When not defined, is NULL.
   */
  struct NaClInstState* cur_inst_state;
  /* Cached instruction. Only guaranteed to be defined when a NaClValidator is
   * called. When not defined, is NULL.
   */
  const struct NaClInst* cur_inst;
  /* Cached translation of instruction. Only guaranteed to be defined when a
   * NaClValidator is called. When not defined, is NULL.
   */
  struct NaClExpVector* cur_inst_vector;
  /* Cached quit value. Kept up to date throughout the lifetime of the
   * validator state. Safe to use within registered validator functions.
   */
  Bool quit;
  /* Define whether we should stub out instructions (i.e. replace with HALT),
   * if they are found to be illegal.
   */
  Bool do_stub_out;
  /* Defines the function to call to apply summary validator rules. Defaults to
   * NaClValidatorRules which applies the post validators functions
   * for sel_ldr (i.e. non-detailed).
   */
  NaClValidatorRulesInitFn rules_init_fn;
};

/* Add validators to validator state if missing. Assumed to be called just
 * before analyzing a code segment. Returns TRUE if able to initialize
 * validators. If FALSE is returned, DO NOT try to run the validator.
 */
Bool NaClValidatorStateInitializeValidators(NaClValidatorState* state);

/* Clean up the state associated with validators. */
void NaClValidatorStateCleanUpValidators(NaClValidatorState* state);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATE_ITER_INTERNAL_H__ */
