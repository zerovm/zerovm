/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * ncvalidate_iter.c
 * Validate x86 instructions for Native Client
 *
 */

#include "src/validator/x86/ncval_reg_sfi/ncvalidate_iter.h"

#include <assert.h>
#include <string.h>

#include "include/portability_io.h"
#include "src/platform/nacl_check.h"
#include "src/platform/nacl_log.h"
#include "src/validator/x86/decoder/ncop_exps.h"
#include "src/validator/x86/decoder/nc_inst_state_internal.h"
#include "src/validator/x86/halt_trim.h"
#include "src/validator/x86/nc_segment.h"
#include "src/validator/x86/ncval_reg_sfi/ncvalidate_iter_internal.h"
#include "src/validator/x86/ncval_reg_sfi/ncval_decode_tables.h"
#include "src/validator/x86/ncval_reg_sfi/nc_illegal.h"
#include "src/validator/x86/ncval_reg_sfi/nc_jumps.h"
#include "src/validator/x86/ncval_reg_sfi/nc_jumps_detailed.h"
#include "src/validator/x86/ncval_reg_sfi/nc_memory_protect.h"
#ifdef NCVAL_TESTING
#include "src/validator/x86/ncval_reg_sfi/nc_postconds.h"
#endif
#include "src/validator_x86/ncdis_decode_tables.h"

/* To turn on debugging of instruction decoding, change value of
 * DEBUGGING to 1.
 */
#define DEBUGGING 0

#include "src/utils/debugging.h"

#include "src/validator/x86/decoder/nc_inst_iter_inl.c"

/* When >= 0, only print that many errors before quiting. When
 * < 0, print all errors.
 */
int NACL_FLAGS_max_reported_errors =
#ifdef NCVAL_TESTING
    /* Turn off error reporting when generating pre/post conditions.
     * Note: conditional code for NCVAL_TESTING will reset the
     * quit flag after each instruction. Hence, by using 0, we
     * effectively turn off printing of errors as the default. However,
     * one can override this on the command line to force errors to
     * be printed as well.
     */
    0
#else
    100
#endif
    ;

Bool NACL_FLAGS_validator_trace_instructions = FALSE;

Bool NACL_FLAGS_validator_trace_inst_internals = FALSE;

Bool NACL_FLAGS_ncval_annotate = TRUE;

#ifdef NCVAL_TESTING
Bool NACL_FLAGS_report_conditions_on_all = FALSE;

void NaClConditionAppend(char* condition,
                         char** buffer,
                         size_t* remaining_buffer_size) {
  size_t bsize = strlen(condition);
  *buffer = condition + bsize;
  if (bsize > 0) {
    /* Add that we are adding an alternative. */
    SNPRINTF(*buffer, NCVAL_CONDITION_SIZE - bsize,
             "&");
    bsize = strlen(condition);
    *buffer = condition + bsize;
  }
  *remaining_buffer_size = NCVAL_CONDITION_SIZE - bsize;
}
#endif

/* Define the stop instruction. */
const uint8_t kNaClFullStop = 0xf4;   /* x86 HALT opcode */

void NaClValidatorFlagsSetTraceVerbose() {
  NACL_FLAGS_validator_trace_instructions = TRUE;
  NACL_FLAGS_validator_trace_inst_internals = TRUE;
}

int NaClValidatorStateGetMaxReportedErrors(NaClValidatorState *vstate) {
  return vstate->quit_after_error_count;
}

void NaClValidatorStateSetMaxReportedErrors(NaClValidatorState *vstate,
                                            int new_value) {
  vstate->quit_after_error_count = new_value;
  vstate->quit = NaClValidatorQuit(vstate);
}

Bool NaClValidatorStateGetTraceInstructions(NaClValidatorState *vstate) {
  return vstate->trace_instructions;
}

void NaClValidatorStateSetTraceInstructions(NaClValidatorState *vstate,
                                            Bool new_value) {
  vstate->trace_instructions = new_value;
}

Bool NaClValidatorStateGetTraceInstInternals(NaClValidatorState *vstate) {
  return vstate->trace_inst_internals;
}

void NaClValidatorStateSetTraceInstInternals(NaClValidatorState *vstate,
                                             Bool new_value) {
  vstate->trace_inst_internals = new_value;
}

static INLINE Bool NaClValidatorStateTraceInline(NaClValidatorState *vstate) {
  return vstate->trace_instructions || vstate->trace_inst_internals;
}


Bool NaClValidatorStateTrace(NaClValidatorState *vstate) {
  return NaClValidatorStateTraceInline(vstate);
}

void NaClValidatorStateSetTraceVerbose(NaClValidatorState *vstate) {
  vstate->trace_instructions = TRUE;
  vstate->trace_inst_internals = TRUE;
}

int NaClValidatorStateGetLogVerbosity(NaClValidatorState *vstate) {
  return vstate->log_verbosity;
}

void NaClValidatorStateSetLogVerbosity(NaClValidatorState *vstate,
                                       Bool new_value) {
  vstate->log_verbosity = new_value;
}

Bool NaClValidatorStateGetDoStubOut(NaClValidatorState *vstate) {
  return vstate->do_stub_out;
}

void NaClValidatorStateSetDoStubOut(NaClValidatorState *vstate,
                                    Bool new_value) {
  vstate->do_stub_out = new_value;
  /* We also turn off error diagnostics, under the assumption
   * you don't want them. (Note: if the user wants them,
   * you can run ncval to get them).
   */
  if (new_value) {
    NaClValidatorStateSetMaxReportedErrors(vstate, 0);
  }
}

static void NaClValidatorTrace(NaClValidatorState* vstate) {
  struct Gio* g = NaClLogGetGio();
  NaClInstState* inst_state = NaClInstIterGetStateInline(vstate->cur_iter);
  (*vstate->error_reporter->printf)
      (vstate->error_reporter, "-> visit: ");
  if (NaClValidatorStateGetTraceInstructions(vstate)) {
    (*vstate->error_reporter->print_inst)(vstate->error_reporter,
                                         (void*) NaClInstStateInst(inst_state));
  }
  if (NaClValidatorStateGetTraceInstInternals(vstate)) {
    NaClExpVectorPrint(g, inst_state);
  }
}

/* TODO(karl) Move the print routines to a separate module. */

/* Returns true if an error message should be printed for the given level, in
 * the current validator state.
 * Parameters:
 *   vstate - The validator state (may be NULL).
 *   level - The log level of the validator message.
 */
static INLINE Bool NaClPrintValidatorMessages(
    NaClValidatorState *vstate, int level) {
  if (NULL == vstate) {
    /* Validator not defined yet, only used log verbosity to decide if message
     * should be printed.
     */
    return level <= NaClLogGetVerbosity();
  } else {
    return (vstate->quit_after_error_count != 0) &&
        (level <= vstate->log_verbosity) &&
        (level <= NaClLogGetVerbosity());
  }
}

static INLINE const char *NaClLogLevelLabel(int level) {
  switch (level) {
    case LOG_WARNING:
      return "WARNING: ";
    case LOG_ERROR:
      return "ERROR: ";
    case LOG_FATAL:
      return "FATAL: ";
    default:
      return "";
  }
}

/* Records that an error message has just been reported.
 * Parameters:
 *   vstate - The validator state (may be NULL).
 *   level - The log level of the validator message.
 */
static void NaClRecordErrorReported(NaClValidatorState *vstate, int level) {
  if ((vstate != NULL) && ((level == LOG_ERROR) || (level == LOG_FATAL)) &&
      (vstate->quit_after_error_count > 0) &&
      !vstate->do_stub_out) {
    --(vstate->quit_after_error_count);
    vstate->quit = NaClValidatorQuit(vstate);
    if (vstate->quit_after_error_count == 0) {
      (*vstate->error_reporter->printf)(
          vstate->error_reporter,
          "%sError limit reached. Validator quitting!\n",
          NaClLogLevelLabel(LOG_INFO));
    }
  }
}

/* Records the number of error validator messages generated for the state.
 * Parameters:
 *   vstate - The validator state (may be NULL).
 *   level - The log level of the validator message.
 * Returns - Updated error level, based on state.
 */
static INLINE int NaClRecordIfValidatorError(NaClValidatorState *vstate,
                                             int level) {
  /* Note: don't quit if stubbing out, so all problems are fixed. */
  if (((level == LOG_ERROR) || (level == LOG_FATAL)) &&
      (NULL != vstate) && !vstate->do_stub_out) {
    vstate->validates_ok = FALSE;
    vstate->quit = NaClValidatorQuit(vstate);
  }
  return level;
}

/* The low-level implementation for stubbing out an instruction. Always use
 * this function to (ultimately) stub out instructions. This makes it possible
 * to detect when the validator modifies the code.
 */
void NCStubOutMem(NaClValidatorState *state, void *ptr, size_t num) {
  state->did_stub_out = TRUE;
  memset(ptr, kNaClFullStop, num);
}

/* Does stub out of instruction in validator state. */
static void NaClStubOutInst(NaClValidatorState *state, NaClInstState* inst) {
  NCRemainingMemory *memory = inst->bytes.memory;
  NCStubOutMem(state, memory->mpc, memory->read_length);
}

/* Does a printf using the error reporter of the state if defined.
 * Uses NaClLogGetGio() if undefined.
 */
static INLINE void NaClPrintVMessage(NaClValidatorState* vstate,
                                     const char* format,
                                     va_list ap) {
  if (vstate) {
    vstate->error_reporter->printf_v(vstate->error_reporter, format, ap);
  } else {
    gvprintf(NaClLogGetGio(), format, ap);
  }
}

/* Forward declaration to define printf arguments. */
static void NaClPrintMessage(NaClValidatorState* vstate,
                             const char* format,
                             ...) ATTRIBUTE_FORMAT_PRINTF(2, 3);

/* Does a printf using the error reporter of the state if defined.
 * Uses NaClLogGetGio() if undefined.
 */
static INLINE void NaClPrintMessage(NaClValidatorState* vstate,
                                    const char* format,
                                    ...) {
  va_list ap;
  va_start(ap, format);
  NaClPrintVMessage(vstate, format, ap);
  va_end(ap);
}

/* Print out a predefined prefix for messages, along with the serverity
 * of the message.
 */
static void NaClPrintValidatorPrefix(int level,
                                     NaClValidatorState* vstate,
                                     Bool visible_level) {
  NaClPrintMessage(vstate, "VALIDATOR: ");
  if (visible_level) NaClPrintMessage(vstate, "%s", NaClLogLevelLabel(level));
}

/* Prints out an instruction on behalf of the validator. */
static void NaClValidatorPrintInst(int level,
                                   NaClValidatorState *vstate,
                                   NaClInstState *inst) {
  NaClPrintValidatorPrefix(level, vstate, FALSE);
  if (vstate) {
    vstate->error_reporter->print_inst(vstate->error_reporter, (void*) inst);
  } else {
    NaClInstStateInstPrint(NaClLogGetGio(), inst);
  }
}

void NaClValidatorMessage(int level,
                          NaClValidatorState *vstate,
                          const char *format,
                          ...) {
  level = NaClRecordIfValidatorError(vstate, level);
  if (NaClPrintValidatorMessages(vstate, level)) {
    va_list ap;
    NaClPrintValidatorPrefix(level, vstate, TRUE);
    va_start(ap, format);
    NaClPrintVMessage(vstate, format, ap);
    va_end(ap);
    NaClRecordErrorReported(vstate, level);
  }
}

void NaClValidatorVarargMessage(int level,
                                NaClValidatorState *vstate,
                                const char *format,
                                va_list ap) {
  level = NaClRecordIfValidatorError(vstate, level);
  if (NaClPrintValidatorMessages(vstate, level)) {
    NaClPrintValidatorPrefix(level, vstate, TRUE);
    NaClPrintVMessage(vstate, format, ap);
    NaClRecordErrorReported(vstate, level);
  }
}

static void NaClValidatorPcAddressMess(
    int level,
    NaClValidatorState *vstate,
    NaClPcAddress addr,
    const char *format,
    va_list ap) {
  level = NaClRecordIfValidatorError(vstate, level);
  if (NaClPrintValidatorMessages(vstate, level)) {
    NaClPrintValidatorPrefix(level, vstate, !NACL_FLAGS_ncval_annotate);
    NaClPrintMessage(vstate, "%"NACL_PRIxNaClPcAddress ": ", addr);
    NaClPrintVMessage(vstate, format, ap);
    NaClRecordErrorReported(vstate, level);
  }
}

void NaClValidatorPcAddressMessage(int level,
                                   NaClValidatorState *vstate,
                                   NaClPcAddress addr,
                                   const char *format,
                                   ...) {
  va_list ap;
  va_start(ap, format);
  NaClValidatorPcAddressMess(level, vstate, vstate->vbase + addr, format, ap);
  va_end(ap);
}

void NaClValidatorInstMessage(int level,
                              NaClValidatorState *vstate,
                              NaClInstState *inst,
                              const char *format,
                              ...) {
  if (NACL_FLAGS_ncval_annotate) {
    va_list ap;
    va_start(ap, format);
    NaClValidatorPcAddressMess(level, vstate,
                               NaClInstStatePrintableAddress(inst),
                               format, ap);
    va_end(ap);
  } else {
    level = NaClRecordIfValidatorError(vstate, level);
    if (NaClPrintValidatorMessages(vstate, level)) {
      va_list ap;
      NaClValidatorPrintInst(level, vstate, inst);
      NaClPrintValidatorPrefix(level, vstate, TRUE);
      va_start(ap, format);
      NaClPrintVMessage(vstate, format, ap);
      va_end(ap);
      NaClRecordErrorReported(vstate, level);
    }
  }
  if (vstate->do_stub_out && (level <= LOG_ERROR)) {
    NaClStubOutInst(vstate, inst);
  }
}

void NaClValidatorTwoInstMessage(int level,
                                 NaClValidatorState *vstate,
                                 NaClInstState *inst1,
                                 NaClInstState *inst2,
                                 const char *format,
                                 ...) {
  level = NaClRecordIfValidatorError(vstate, level);
  if (NaClPrintValidatorMessages(vstate, level)) {
    va_list ap;
    NaClPrintValidatorPrefix(level, vstate, TRUE);
    va_start(ap, format);
    NaClPrintVMessage(vstate, format, ap);
    va_end(ap);
    NaClPrintMessage(vstate, "\n                                   ");
    NaClValidatorPrintInst(level, vstate, inst1);
    NaClPrintMessage(vstate, "                                   ");
    NaClValidatorPrintInst(level, vstate, inst2);
    NaClRecordErrorReported(vstate, level);
  }
  if (vstate->do_stub_out && (level <= LOG_ERROR)) {
    NaClStubOutInst(vstate, inst2);
  }
}

Bool NaClValidatorQuit(NaClValidatorState *vstate) {
  return !vstate->validates_ok && (vstate->quit_after_error_count == 0);
}

Bool NaClValidatorDidStubOut(NaClValidatorState *vstate) {
  return vstate->did_stub_out;
}

static void NaClNullErrorPrintInst(NaClErrorReporter* self,
                                   struct NaClInstState* inst) {}

NaClErrorReporter kNaClNullErrorReporter = {
  NaClNullErrorReporter,
  NaClNullErrorPrintf,
  NaClNullErrorPrintfV,
  (NaClPrintInst) NaClNullErrorPrintInst
};

/* Update caches associated the current instruction state associated with
 * the iterator of the validator state. This routine should be called everytime
 * the iterator (of the validator state) is advanced.
 */
static INLINE void NaClUpdateCaches(NaClValidatorState *vstate) {
#ifdef NCVAL_TESTING
  /* Initialize the pre/post conditions to the empty condition. */
  strcpy(&vstate->precond[0], "");
  strcpy(&vstate->postcond[0], "");
#endif
  vstate->cur_inst_state = NaClInstIterGetStateInline(vstate->cur_iter);
  vstate->cur_inst = NaClInstStateInst(vstate->cur_inst_state);
  vstate->cur_inst_vector = NaClInstStateExpVector(vstate->cur_inst_state);
}

/* Returns true if the current position of the instruction iterator (for
 * the validator state) points to an instruction (i.e. not at the end of
 * the segment).
 */
static INLINE Bool NaClValidatorStateIterHasNextInline(
    NaClValidatorState *vstate) {
  Bool result;
  if (NULL == vstate->cur_iter) return FALSE;
  result = NaClInstIterHasNextInline(vstate->cur_iter);
  if (result && NULL == vstate->cur_inst_state) {
    /* If reached, this is the first query to check if there is
     * a next instruction. Update the caches to match.
     */
    NaClUpdateCaches(vstate);
  }
  return result;
}

Bool NaClValidatorStateIterHasNext(NaClValidatorState *vstate) {
  return NaClValidatorStateIterHasNextInline(vstate);
}

/* Move past the current instruction defined by the iterator of the
 * validator state.
 */
static INLINE void NaClValidatorStateIterAdvanceInline(
    NaClValidatorState *vstate) {
  NaClInstIterAdvanceInline(vstate->cur_iter);
  NaClUpdateCaches(vstate);
}

void NaClValidatorStateIterAdvance(NaClValidatorState *vstate) {
  NaClValidatorStateIterAdvanceInline(vstate);
}

/* Iterator of the validator state is no longer needed, clean up any
 * caches associated with the iterator.
 */
static void NaClValidatorStateIterFinishInline(NaClValidatorState *vstate) {
  vstate->cur_inst_state = NULL;
  vstate->cur_inst = NULL;
  vstate->cur_inst_vector = NULL;
}

void NaClValidatorStateIterFinish(NaClValidatorState *vstate) {
  NaClValidatorStateIterFinishInline(vstate);
}

NaClValidatorState *NaClValidatorStateCreate(
    const NaClPcAddress vbase,
    const NaClMemorySize codesize,
    const NaClOpKind base_register,
    const int readonly_text,
    const NaClCPUFeaturesX86 *features) {
  NaClValidatorState *vstate;
  NaClValidatorState *return_value = NULL;
  const int bundle_size = 32;
  DEBUG(NaClLog(LOG_INFO,
                "Validator Create: vbase = %"NACL_PRIxNaClPcAddress", "
                "sz = %"NACL_PRIxNaClMemorySize", bundle_size = %u\n",
                vbase, codesize, bundle_size));
  if (features == NULL)
    return NULL;
  vstate = (NaClValidatorState*) malloc(sizeof(NaClValidatorState));
  if (vstate != NULL) {
    return_value = vstate;
    vstate->decoder_tables = kNaClValDecoderTables;
    vstate->vbase = vbase;
    vstate->bundle_size = bundle_size;
    vstate->codesize = codesize;
    vstate->bundle_mask = bundle_size - 1;
    NaClCopyCPUFeatures(&vstate->cpu_features, features);
    vstate->base_register = base_register;
    vstate->validates_ok = TRUE;
    vstate->did_stub_out = FALSE;
    vstate->quit_after_error_count = NACL_FLAGS_max_reported_errors;
#ifdef NCVAL_TESTING
    vstate->validates_ok_with_conditions = TRUE;
    vstate->report_conditions_on_all = NACL_FLAGS_report_conditions_on_all;
#endif
    vstate->error_reporter = &kNaClNullErrorReporter;
    vstate->print_opcode_histogram = NACL_FLAGS_opcode_histogram;
    vstate->trace_instructions = NACL_FLAGS_validator_trace_instructions;
    vstate->trace_inst_internals = NACL_FLAGS_validator_trace_inst_internals;
    vstate->log_verbosity = LOG_INFO;
    vstate->cur_iter = NULL;
    NaClValidatorStateIterFinishInline(vstate);
    vstate->quit = NaClValidatorQuit(return_value);
    vstate->do_stub_out = FALSE;
    vstate->readonly_text = readonly_text;
    vstate->do_detailed = FALSE;
    NaClOpcodeHistogramInitialize(vstate);
    NaClCpuCheckMemoryInitialize(vstate);
    NaClBaseRegisterMemoryInitialize(vstate);
    if (!NaClJumpValidatorInitialize(vstate)) {
      NaClValidatorStateDestroy(vstate);
      return_value = NULL;
    }
  }
  return return_value;
}

Bool NaClValidatorStateIterReset(NaClValidatorState *vstate) {
  /* Record infromation needed to reset the iterator, based on the
   * current iterator
   */
  size_t lookback_size = lookback_size = vstate->cur_iter->buffer_size;
  NaClSegment* segment = vstate->cur_iter->segment;

  if (NULL == vstate->cur_iter) return FALSE;

  /* Before deleting, be sure to clean up cached information. and
   * the destroy the current validator. */
  NaClValidatorStateIterFinishInline(vstate);
  NaClInstIterDestroy(vstate->cur_iter);

  /* Now create a new instruction iterator. */
  vstate->cur_iter = NaClInstIterCreateWithLookback(
      vstate->decoder_tables, segment, lookback_size);

  if (NULL == vstate->cur_iter) return FALSE;
  return TRUE;
}

#ifdef NCVAL_TESTING
void NaClPrintConditions(NaClValidatorState *state) {
  /* To save space, only report on instructions that have non-empty
   * pre/post conditions.
   */
  if ((strlen(state->precond) > 0) || (strlen(state->postcond) > 0)) {
    printf("%"NACL_PRIxNaClPcAddress": ",
           NaClInstStatePrintableAddress(state->cur_inst_state));
    if ('\0' != state->precond[0]) {
      printf("%s", state->precond);
    }
    if ('\0' != state->postcond[0]) {
      if ('\0' != state->precond[0]) printf(" ");
      printf("-> %s", state->postcond);
    }
    printf("\n");
  }
}
#endif

/* Given we are at the instruction defined by the instruction iterator, for
 * a segment, apply all applicable validator functions.
 */
static INLINE void NaClApplyValidators(NaClValidatorState *vstate) {
  if (vstate->quit) return;
  DEBUG(NaClLog(LOG_INFO, "iter state:\n");
        NaClInstStateInstPrint(NaClLogGetGio(),
                               NaClInstIterGetState(vstate->cur_iter)));
  if (NaClValidatorStateTraceInline(vstate)) {
    NaClValidatorTrace(vstate);
  }
  NaClCpuCheck(vstate, vstate->cur_iter);
  NaClValidateInstructionLegal(vstate);
  NaClBaseRegisterValidator(vstate);
  NaClMemoryReferenceValidator(vstate);
  NaClJumpValidator(vstate);
  if (vstate->print_opcode_histogram) {
    NaClOpcodeHistogramRecord(vstate);
  }
#ifdef NCVAL_TESTING
  /* Collect post conditions for instructions that are non-last.
   * Only print pre/post conditions for valid instructions (ignoring
   * pre/post conditions).
   */
  if (NaClValidatesOk(vstate) || vstate->report_conditions_on_all) {
    NaClAddAssignsRegisterWithZeroExtendsPostconds(vstate);
    NaClAddLeaSafeAddressPostconds(vstate);
    NaClPrintConditions(vstate);
  }
  /* Reset the exit flags, so that errors do not stop
   * other instructions from firing errors. By reseting these flags,
   * it allows us to only print pre/post conditions for instructions
   * that are not marked illegal.
   */
  if (!vstate->validates_ok) vstate->validates_ok_with_conditions = FALSE;
  vstate->validates_ok = TRUE;
  vstate->quit = FALSE;
#endif
}

/* Given that we have just iterated through all instructions in a segment,
 * apply post validators rules (before we collect the iterator).
 */
static INLINE void NaClApplyPostValidators(NaClValidatorState *vstate) {
  DEBUG(NaClLog(LOG_INFO, "applying post validators...\n"));
  if (vstate->quit || (NULL == vstate->cur_iter)) return;

  /* Before doing anything else, process remaining (forward) information
   * stored by the validator, to see if any errors should be reported
   * about the last instruction processed.
   */
  NaClBaseRegisterSummarize(vstate);

  /* Now do the summarizing steps of the validator. */
  if (vstate->do_detailed) {
    NaClJumpValidatorSummarizeDetailed(vstate);
  } else {
    NaClJumpValidatorSummarize(vstate);
  }
  if (NaClValidatorStateTrace(vstate)) {
    (vstate->error_reporter->printf)
        (vstate->error_reporter, "<- visit\n");
  }
}

/* The maximum lookback for the instruction iterator of the segment.
 * Note: Allows for two memory patterns (4 instructions for each pattern).
 */
static const size_t kLookbackSize = 8;

void NaClValidateSegment(uint8_t *mbase, NaClPcAddress vbase,
                         NaClMemorySize size, NaClValidatorState *vstate) {
  NaClSegment segment;
  do {
    /* Sanity checks */
    /* TODO(ncbray): remove redundant vbase/size args. */
    if ((vbase & vstate->bundle_mask) != 0) {
      NaClValidatorMessage(LOG_ERROR, vstate,
                           "Code segment starts at 0x%"NACL_PRIxNaClPcAddress
                           ", which isn't aligned properly.\n",
                           vbase);
      break;
    }
    if (vbase != vstate->vbase) {
      NaClValidatorMessage(LOG_ERROR, vstate, "Mismatched vbase address\n");
      break;
    }
    if (size != vstate->codesize) {
      NaClValidatorMessage(LOG_ERROR, vstate, "Mismatched code size\n");
      break;
    }
    if (vbase > vbase + size) {
      NaClValidatorMessage(LOG_ERROR, vstate, "Text segment too big for given "
                           "vbase (address overflow)\n");
      break;
    }

    size = NCHaltTrimSize(mbase, size, vstate->bundle_size);
    vstate->codesize = size;

    if (size == 0) {
      NaClValidatorMessage(LOG_ERROR, vstate, "Bad text segment (zero size)\n");
      break;
    }

    NaClSegmentInitialize(mbase, vbase, size, &segment);

    vstate->cur_iter = NaClInstIterCreateWithLookback(vstate->decoder_tables,
                                                      &segment, kLookbackSize);
    if (NULL == vstate->cur_iter) {
      NaClValidatorMessage(LOG_ERROR, vstate, "Not enough memory\n");
      break;
    }
    for (; NaClValidatorStateIterHasNextInline(vstate);
         NaClValidatorStateIterAdvanceInline(vstate)) {
      NaClApplyValidators(vstate);
      if (vstate->quit) break;
    }
    NaClValidatorStateIterFinish(vstate);
  } while (0);
  NaClApplyPostValidators(vstate);
  NaClInstIterDestroy(vstate->cur_iter);
  vstate->cur_iter = NULL;
  if (vstate->print_opcode_histogram) {
    NaClOpcodeHistogramPrintStats(vstate);
  }
#ifdef NCVAL_TESTING
  /* Update failure to catch instructions that may have validated
   * incorrectly.
   */
  if (vstate->validates_ok)
    vstate->validates_ok = vstate->validates_ok_with_conditions;
#endif
}

void NaClValidateSegmentUsingTables(uint8_t* mbase,
                                    NaClPcAddress vbase,
                                    NaClMemorySize sz,
                                    NaClValidatorState* vstate,
                                    const struct NaClDecodeTables* tables) {
  vstate->decoder_tables = tables;
  NaClValidateSegment(mbase, vbase, sz, vstate);
}

Bool NaClValidatesOk(NaClValidatorState *vstate) {
  return vstate->validates_ok;
}

void NaClValidatorStateDestroy(NaClValidatorState *vstate) {
  if (NULL != vstate) {
    NaClJumpValidatorCleanUp(vstate);
    free(vstate);
  }
}

/*
 * Check that iter_new is a valid replacement for iter_old.
 * If a validation error occurs, vstate->validates_ok will be set to false by
 * NaClValidatorInstMessage when it is given LOG_ERROR, see the end of this
 * function.
 * Return value: TRUE if the instruction was changed, FALSE if it's identical.
 */
static Bool NaClValidateInstReplacement(NaClInstIter *iter_old,
                                        NaClInstIter *iter_new,
                                        struct NaClValidatorState *vstate) {
  NaClInstState *istate_old, *istate_new;
  NaClExpVector *exp_old, *exp_new;
  uint32_t i;
  Bool inst_changed = FALSE;
  int parent_index;

  istate_old = NaClInstIterGetStateInline(iter_old);
  istate_new = NaClInstIterGetStateInline(iter_new);

  /* Location/length must match.  Assumes vbase is the same. */
  if (istate_new->inst_addr != istate_old->inst_addr ||
      istate_new->bytes.length != istate_old->bytes.length) {
    NaClValidatorTwoInstMessage(LOG_ERROR, vstate, istate_old, istate_new,
          "Code modification: instructions length/addresses do not match");
    inst_changed = TRUE;
    return inst_changed;
  }


  do {
    /* fast check if the replacement is identical */
    if ((istate_old->bytes.memory->read_length ==
         istate_new->bytes.memory->read_length) &&
        !memcmp(istate_old->bytes.memory->mpc, istate_new->bytes.memory->mpc,
                istate_old->bytes.memory->read_length))
      return inst_changed;

    inst_changed = TRUE;

    if (istate_old->num_prefix_bytes != istate_new->num_prefix_bytes)
      break;
    if (istate_old->num_rex_prefixes != istate_new->num_rex_prefixes)
      break;
    if (istate_old->rexprefix != istate_new->rexprefix)
      break;
    if (istate_old->modrm != istate_new->modrm)
      break;
    if (istate_old->has_sib != istate_new->has_sib)
      break;
    if (istate_old->has_sib && istate_old->sib != istate_new->sib)
      break;
    if (istate_old->operand_size != istate_new->operand_size)
      break;
    if (istate_old->address_size != istate_new->address_size)
      break;
    if (istate_old->prefix_mask != istate_new->prefix_mask)
      break;

    /*
     * these are pointers, but they reference entries in a static table,
     * so if the two instructions are the same, then these pointers must
     * reference the same entry
     */
    if (istate_old->inst != istate_new->inst)
      break;

    exp_old = NaClInstStateExpVector(istate_old);
    exp_new = NaClInstStateExpVector(istate_new);

    /* check if the instruction operands are identical */
    if (exp_old->number_expr_nodes != exp_new->number_expr_nodes)
      break;

    for (i = 0; i < exp_old->number_expr_nodes; i++) {
      /* Allow nodes that are identical. */
      if (exp_old->node[i].kind  != exp_new->node[i].kind) goto error_exit;
      if (exp_old->node[i].flags != exp_new->node[i].flags) goto error_exit;
      if (exp_old->node[i].value == exp_new->node[i].value) continue;

      /*
       * Only constant values may differ. However it is important not to
       * allow constant modification of sandboxing instructions. Note neither
       * of the instructions allowed for modification is used for sandboxing.
       */
      if (exp_old->node[i].kind != ExprConstant) goto error_exit;

      switch (istate_old->inst->name) {

        case InstCall:
          /* allow different constants in direct calls */
          if (!NaClHasBit(exp_old->node[i].flags, NACL_EFLAG(ExprJumpTarget)))
            break;
          parent_index = NaClGetExpParentIndex(exp_old, i);
          if (parent_index < 0) break;
          if (exp_old->node[parent_index].kind == OperandReference) continue;
          break;

        case InstMov:
          parent_index = NaClGetExpParentIndex(exp_old, i);
          if (parent_index < 0) break;
          switch (exp_old->node[parent_index].kind) {
            case OperandReference:
              /*
               * allow different constants in operand of mov
               * e.g. mov $rax, 0xdeadbeef
               */
              if (NaClHasBit(exp_old->node[i].flags, NACL_EFLAG(ExprUsed)))
                continue;
              break;
            case ExprMemOffset:
              /*
               * allow different displacements in memory reference of mov
               * instructions e.g. mov $rax, [$r15+$rbx*2+0x7fff]
               *
               * Note: displacement is the fourth node after ExprMemOffset*
               * node
               */
              if (4 == (i - parent_index)) continue;
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }

      /* If reached, we found a value that differed, and wasn't one
       * of the expected constants that can differ.
       */
      goto error_exit;
    }

    /* This return signifies there is no error in validation. */
    return inst_changed;
  } while (0);

error_exit:
  /* This logging function is the mechanism that sets the validator state
   * to indicate an error.
   */
  NaClValidatorTwoInstMessage(LOG_ERROR, vstate, istate_old, istate_new,
                      "Code modification: failed to modify instruction");
  return inst_changed;
}

/*
 * Validate a new code block as a replacement for an existing block.
 * Note: The code location (vbase) must be bundle aligned and the segment size
 * must also be a multiple of bundle size (checked in
 * NaClValidateCodeReplacement).This is to ensure all NaCl psuedo-instructions
 * and guard sequences are properly inspected and maintained.
 */
void NaClValidateSegmentPair(uint8_t *mbase_old, uint8_t *mbase_new,
                           NaClPcAddress vbase, size_t size,
                           struct NaClValidatorState *vstate) {
  NaClSegment segment_old, segment_new;
  NaClInstIter *iter_old = NULL;
  NaClInstIter *iter_new = NULL;

  NaClSegmentInitialize(mbase_old, vbase, size, &segment_old);
  NaClSegmentInitialize(mbase_new, vbase, size, &segment_new);
  do {
    iter_old = NaClInstIterCreateWithLookback(vstate->decoder_tables,
                                              &segment_old, kLookbackSize);
    if (NULL == iter_old) break;
    iter_new = NaClInstIterCreateWithLookback(vstate->decoder_tables,
                                              &segment_new, kLookbackSize);
    if (NULL == iter_new) break;
    vstate->cur_iter = iter_new;
    while (NaClInstIterHasNextInline(iter_old) &&
           NaClValidatorStateIterHasNextInline(vstate)) {
      vstate->cur_inst_state->unchanged =
          !NaClValidateInstReplacement(iter_old, iter_new, vstate);
      NaClApplyValidators(vstate);
      if (vstate->quit) break;
      NaClInstIterAdvanceInline(iter_old);
      NaClValidatorStateIterAdvanceInline(vstate);
    }
    if (NaClInstIterHasNextInline(iter_old) ||
        NaClInstIterHasNextInline(iter_new)) {
      NaClValidatorMessage(
          LOG_ERROR, vstate,
          "Code modification: code segments have different "
          "number of instructions\n");
    }
  } while (0);
  NaClValidatorStateIterFinish(vstate);
  NaClApplyPostValidators(vstate);
  vstate->cur_iter = NULL;
  NaClInstIterDestroy(iter_old);
  NaClInstIterDestroy(iter_new);
}
