/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATE_ITER_INTERNAL_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATE_ITER_INTERNAL_H__

/* Defines the internal data structure for the validator state. */

#include "src/utils/types.h"
#include "src/validator/x86/nacl_cpuid.h"
#include "src/validator/x86/ncval_reg_sfi/nc_cpu_checks.h"
#include "src/validator/x86/ncval_reg_sfi/nc_jumps.h"
#include "src/validator/x86/ncval_reg_sfi/nc_opcode_histogram.h"
#include "src/validator/x86/ncval_reg_sfi/nc_protect_base.h"

struct NaClDecodeTables;
struct NaClExpVector;
struct NaClInst;
struct NaClInstIter;
struct NaClInstState;
struct NaClValidatorState;

#ifdef NCVAL_TESTING
/* Maximum size for pre/post conditions (as strings). */
#define NCVAL_CONDITION_SIZE 1024
#endif

struct NaClValidatorState {
  /* Holds the decoder tables to use. */
  const struct NaClDecodeTables* decoder_tables;
  /* Holds the vbase value passed to NaClValidatorStateCreate. */
  NaClPcAddress vbase;
  /* Holds the size value passed to NaClValidatorStateCreate. */
  NaClMemorySize codesize;
  /* Holds the bundle size value passed to NaClValidatorStateCreate. */
  uint8_t bundle_size;
  /* Holds the bundle mask, which when applied to an address catches any lower
   * bits that violate alignment.
   */
  NaClPcAddress bundle_mask;
  /* Holds the value for the base register, or RegUnknown if undefined. */
  NaClOpKind base_register;
  /* Holds if the validation is still valid. */
  Bool validates_ok;
#ifdef NCVAL_TESTING
  /* Hold if any problems occured during validation of any instruction.
   * This is needed when generating pre/post conditions because we
   * need to override validates_ok on each instruction so that we
   * will generate pre/post conditions for all instructions.
   */
  Bool validates_ok_with_conditions;
#endif
  /* Holds if any stubouts have been performed. */
  Bool did_stub_out;
  /* If >= 0, holds how many errors can be reported. If negative,
   * reports all errors.
   */
  int quit_after_error_count;
#ifdef NCVAL_TESTING
  /* Define whether we should report pre/post conditions, even
   * if the instruction does not validate.
   */
  Bool report_conditions_on_all;
#endif
  /* Holds the error reporting object to use. */
  NaClErrorReporter* error_reporter;
  /* Holds the cpu features of the machine it is running on. */
  NaClCPUFeaturesX86 cpu_features;
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
  /* The iterator currently being used, or NULL if no such iterator. */
  struct NaClInstIter* cur_iter;
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
  /* Define whether the text segment should be handled as read-only.
   */
  Bool readonly_text;
  /* When true, generate detailed error messages instead of summary messages. */
  Bool do_detailed;
  /* Defines the local data needed while analyzing jumps and instruction
   * boundaries.
   */
  NaClJumpSets jump_sets;
  /* Defines the locals used to record registers set in the current instruction,
   * that are a problem if not used correctly in the next instruction.
   */
  NaClBaseRegisterLocals set_base_registers;
  /* Defines the set of cpu features aplpied to the code. */
  NaClCpuCheckState cpu_checks;
  /* Defines the collected opcode histogram data. */
  NaClOpcodeHistogram opcode_histogram;
#ifdef NCVAL_TESTING
  /* The string containing validator preconditions. */
  char precond[NCVAL_CONDITION_SIZE];
  /* The string containing validator postconditions. */
  char postcond[NCVAL_CONDITION_SIZE];
#endif
};

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATE_ITER_INTERNAL_H__ */
