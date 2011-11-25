/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidator_registry.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_state.h"
#include "native_client/src/trusted/validator/x86/decoder/ncop_exps.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter_internal.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_jumps.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_cpu_checks.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_illegal.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_jumps.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_opcode_histogram.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_memory_protect.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_protect_base.h"

#include "native_client/src/trusted/validator/x86/decoder/nc_inst_iter_inl.c"

Bool NACL_FLAGS_opcode_histogram = FALSE;

static void NaClValidatorTrace(NaClValidatorState* state,
                               NaClInstIter* iter,
                               void* local_memory) {
  struct Gio* g = NaClLogGetGio();
  NaClInstState* inst_state = NaClInstIterGetStateInline(iter);
  if (NaClValidatorStateTrace(state)) {
    gprintf(g, "-> visit: ");
  }
  if (NaClValidatorStateGetTraceInstructions(state)) {
    (*state->error_reporter->print_inst)(state->error_reporter,
                                         (void*) NaClInstStateInst(inst_state));
  }
  if (NaClValidatorStateGetTraceInstInternals(state)) {
    NaClExpVectorPrint(g, NaClInstStateExpVector(inst_state));
  }
}

static void NaClValidatorPostTrace(NaClValidatorState* state,
                                   NaClInstIter* iter,
                                   void* local_memory) {
  if (NaClValidatorStateTrace(state)) {
    gprintf(NaClLogGetGio(), "<- visit\n");
  }
}

void NaClValidatorRulesInit(NaClValidatorState* state) {

  if (NaClValidatorStateTrace(state)) {
    NaClRegisterValidator(
        state,
        (NaClValidator) NaClValidatorTrace,
        (NaClValidatorPostValidate) NaClValidatorPostTrace,
        (NaClValidatorPrintStats) NULL,
        (NaClValidatorMemoryCreate) NULL,
        (NaClValidatorMemoryDestroy) NULL);
  }

  NaClRegisterValidator(
      state,
      (NaClValidator) NaClCpuCheck,
      (NaClValidatorPostValidate) NULL,
      (NaClValidatorPrintStats) NULL,
      (NaClValidatorMemoryCreate) NaClCpuCheckMemoryCreate,
      (NaClValidatorMemoryDestroy) NaClCpuCheckMemoryDestroy);

  NaClRegisterValidator(
      state,
      (NaClValidator) NaClValidateInstructionLegal,
      (NaClValidatorPostValidate) NULL,
      (NaClValidatorPrintStats) NULL,
      (NaClValidatorMemoryCreate) NULL,
      (NaClValidatorMemoryDestroy) NULL);

  NaClRegisterValidator(
      state,
      (NaClValidator) NaClBaseRegisterValidator,
      (NaClValidatorPostValidate) NaClBaseRegisterSummarize,
      (NaClValidatorPrintStats) NULL,
      (NaClValidatorMemoryCreate) NaClBaseRegisterMemoryCreate,
      (NaClValidatorMemoryDestroy) NaClBaseRegisterMemoryDestroy);

  NaClRegisterValidator(
      state,
      (NaClValidator) NaClMemoryReferenceValidator,
      (NaClValidatorPostValidate) NULL,
      (NaClValidatorPrintStats) NULL,
      (NaClValidatorMemoryCreate) NULL,
      (NaClValidatorMemoryDestroy) NULL);

  /* Thie comes last, since we want error messages that may come
   * from other summarizers (i.e. NaClBaseRegisterSummarize in
   * particular) to come before the summarization error messages
   * of NaClJumpValidatorSummarize.
   */
  NaClRegisterValidator(
      state,
      (NaClValidator) NaClJumpValidator,
      (NaClValidatorPostValidate) NaClJumpValidatorSummarize,
      (NaClValidatorPrintStats) NULL,
      (NaClValidatorMemoryCreate) NaClJumpValidatorCreate,
      (NaClValidatorMemoryDestroy) NaClJumpValidatorDestroy);

  if (state->print_opcode_histogram) {
    NaClRegisterValidator(
        state,
        (NaClValidator) NaClOpcodeHistogramRecord,
        (NaClValidatorPostValidate) NULL,
        (NaClValidatorPrintStats) NaClOpcodeHistogramPrintStats,
        (NaClValidatorMemoryCreate) NaClOpcodeHistogramMemoryCreate,
        (NaClValidatorMemoryDestroy) NaClOpcodeHistogramMemoryDestroy);
  }
}
