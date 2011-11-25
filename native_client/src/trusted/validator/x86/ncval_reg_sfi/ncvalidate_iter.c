/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * ncvalidate_iter.c
 * Validate x86 instructions for Native Client
 *
 */

#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter.h"

#include <assert.h>
#include <string.h>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/validator/x86/decoder/ncop_exps.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_state_internal.h"
#include "native_client/src/trusted/validator/x86/halt_trim.h"
#include "native_client/src/trusted/validator/x86/nc_segment.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter_internal.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidator_registry.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncval_decode_tables.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/nc_jumps.h"
#include "native_client/src/trusted/validator_x86/ncdis_decode_tables.h"

/* To turn on debugging of instruction decoding, change value of
 * DEBUGGING to 1.
 */
#define DEBUGGING 0

#include "native_client/src/shared/utils/debugging.h"

#include "native_client/src/trusted/validator/x86/decoder/nc_inst_iter_inl.c"

/* When >= 0, only print that many errors before quiting. When
 * < 0, print all errors.
 */
int NACL_FLAGS_max_reported_errors = 100;

Bool NACL_FLAGS_validator_trace_instructions = FALSE;

Bool NACL_FLAGS_validator_trace_inst_internals = FALSE;

Bool NACL_FLAGS_ncval_annotate = TRUE;

/* The set of cpu features to use, if non-NULL.
 * NOTE: This global is used to allow the injection of
 * a command-line override of CPU features, from that of the local
 * CPU id, for the tool ncval. As such, when this global is non-null,
 * it uses the injected value this pointer points to as the corresponding
 * CPU id results to use.
 */
static CPUFeatures *nacl_validator_features = NULL;

void NaClValidateSetCPUFeatures(CPUFeatures *new_features) {
  nacl_validator_features = new_features;
}

/* Define the stop instruction. */
const uint8_t kNaClFullStop = 0xf4;   /* x86 HALT opcode */

void NaClValidatorFlagsSetTraceVerbose() {
  NACL_FLAGS_validator_trace_instructions = TRUE;
  NACL_FLAGS_validator_trace_inst_internals = TRUE;
}

int NaClValidatorStateGetMaxReportedErrors(NaClValidatorState *state) {
  return state->quit_after_error_count;
}

void NaClValidatorStateSetMaxReportedErrors(NaClValidatorState *state,
                                            int new_value) {
  state->quit_after_error_count = new_value;
  state->quit = NaClValidatorQuit(state);
}

Bool NaClValidatorStateGetPrintOpcodeHistogram(NaClValidatorState *state) {
  return state->print_opcode_histogram;
}

void NaClValidatorStateSetPrintOpcodeHistogram(NaClValidatorState *state,
                                               Bool new_value) {
  state->print_opcode_histogram = new_value;
}

Bool NaClValidatorStateGetTraceInstructions(NaClValidatorState *state) {
  return state->trace_instructions;
}

void NaClValidatorStateSetTraceInstructions(NaClValidatorState *state,
                                            Bool new_value) {
  state->trace_instructions = new_value;
}

Bool NaClValidatorStateGetTraceInstInternals(NaClValidatorState *state) {
  return state->trace_inst_internals;
}

void NaClValidatorStateSetTraceInstInternals(NaClValidatorState *state,
                                             Bool new_value) {
  state->trace_inst_internals = new_value;
}

Bool NaClValidatorStateTrace(NaClValidatorState *state) {
  return state->trace_instructions || state->trace_inst_internals;
}

void NaClValidatorStateSetTraceVerbose(NaClValidatorState *state) {
  state->trace_instructions = TRUE;
  state->trace_inst_internals = TRUE;
}

int NaClValidatorStateGetLogVerbosity(NaClValidatorState *state) {
  return state->log_verbosity;
}

void NaClValidatorStateSetLogVerbosity(NaClValidatorState *state,
                                       Bool new_value) {
  state->log_verbosity = new_value;
}

Bool NaClValidatorStateGetDoStubOut(NaClValidatorState *state) {
  return state->do_stub_out;
}

void NaClValidatorStateSetDoStubOut(NaClValidatorState *state,
                                    Bool new_value) {
  state->do_stub_out = new_value;
  /* We also turn off error diagnostics, under the assumption
   * you don't want them. (Note: if the user wants them,
   * you can run ncval to get them).
   */
  if (new_value) {
    NaClValidatorStateSetMaxReportedErrors(state, 0);
  }
}

void NaClValidatorStateSetCPUFeatures(NaClValidatorState* state,
                                      const CPUFeatures* features) {
  NaClCopyCPUFeatures(&state->cpu_features, features);
}

/* TODO(karl) Move the print routines to a separate module. */

/* Returns true if an error message should be printed for the given level, in
 * the current validator state.
 * Parameters:
 *   state - The validator state (may be NULL).
 *   level - The log level of the validator message.
 */
static Bool NaClPrintValidatorMessages(NaClValidatorState *state, int level) {
  if (NULL == state) {
    /* Validator not defined yet, only used log verbosity to decide if message
     * should be printed.
     */
    return level <= NaClLogGetVerbosity();
  } else {
    return (state->quit_after_error_count != 0) &&
        (level <= state->log_verbosity) &&
        (level <= NaClLogGetVerbosity());
  }
}

static const char *NaClLogLevelLabel(int level) {
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
 *   state - The validator state (may be NULL).
 *   level - The log level of the validator message.
 */
static void NaClRecordErrorReported(NaClValidatorState *state, int level) {
  if ((state != NULL) && ((level == LOG_ERROR) || (level == LOG_FATAL)) &&
      (state->quit_after_error_count > 0) &&
      !state->do_stub_out) {
    --(state->quit_after_error_count);
    state->quit = NaClValidatorQuit(state);
    if (state->quit_after_error_count == 0) {
      (*state->error_reporter->printf)(
          state->error_reporter,
          "%sError limit reached. Validator quitting!\n",
          NaClLogLevelLabel(LOG_INFO));
    }
  }
}

/* Records the number of error validator messages generated for the state.
 * Parameters:
 *   state - The validator state (may be NULL).
 *   level - The log level of the validator message.
 * Returns - Updated error level, based on state.
 */
static int NaClRecordIfValidatorError(NaClValidatorState *state,
                                             int level) {
  /* Note: don't quit if stubbing out, so all problems are fixed. */
  if (((level == LOG_ERROR) || (level == LOG_FATAL)) &&
      (NULL != state) && !state->do_stub_out) {
    state->validates_ok = FALSE;
    state->quit = NaClValidatorQuit(state);
  }
  return level;
}

/* Does stub out of instruction in validator state. */
static void NaClStubOutInst(NaClInstState* inst) {
  NCRemainingMemory *memory = inst->bytes.memory;
  memset(memory->mpc, kNaClFullStop, memory->read_length);
}

/* Does a printf using the error reporter of the state if defined.
 * Uses NaClLogGetGio() if undefined.
 */
static void NaClPrintVMessage(NaClValidatorState* state,
                              const char* format,
                              va_list ap) {
  if (state) {
    state->error_reporter->printf_v(state->error_reporter, format, ap);
  } else {
    gvprintf(NaClLogGetGio(), format, ap);
  }
}

/* Forward declaration to define printf arguments. */
static void NaClPrintMessage(NaClValidatorState* state,
                             const char* format,
                             ...) ATTRIBUTE_FORMAT_PRINTF(2, 3);

/* Does a printf using the error reporter of the state if defined.
 * Uses NaClLogGetGio() if undefined.
 */
static void NaClPrintMessage(NaClValidatorState* state,
                             const char* format,
                             ...) {
  va_list ap;
  va_start(ap, format);
  NaClPrintVMessage(state, format, ap);
  va_end(ap);
}

/* Print out a predefined prefix for messages, along with the serverity
 * of the message.
 */
static void NaClPrintValidatorPrefix(int level,
                                     NaClValidatorState* state,
                                     Bool visible_level) {
  NaClPrintMessage(state, "VALIDATOR: ");
  if (visible_level) NaClPrintMessage(state, "%s", NaClLogLevelLabel(level));
}

/* Prints out an instruction on behalf of the validator. */
static void NaClValidatorPrintInst(int level,
                                   NaClValidatorState *state,
                                   NaClInstState *inst) {
  NaClPrintValidatorPrefix(level, state, FALSE);
  if (state) {
    state->error_reporter->print_inst(state->error_reporter, (void*) inst);
  } else {
    NaClInstStateInstPrint(NaClLogGetGio(), inst);
  }
}

void NaClValidatorMessage(int level,
                          NaClValidatorState *state,
                          const char *format,
                          ...) {
  level = NaClRecordIfValidatorError(state, level);
  if (NaClPrintValidatorMessages(state, level)) {
    va_list ap;
    NaClPrintValidatorPrefix(level, state, TRUE);
    va_start(ap, format);
    NaClPrintVMessage(state, format, ap);
    va_end(ap);
    NaClRecordErrorReported(state, level);
  }
}

void NaClValidatorVarargMessage(int level,
                                NaClValidatorState *state,
                                const char *format,
                                va_list ap) {
  level = NaClRecordIfValidatorError(state, level);
  if (NaClPrintValidatorMessages(state, level)) {
    NaClPrintValidatorPrefix(level, state, TRUE);
    NaClPrintVMessage(state, format, ap);
    NaClRecordErrorReported(state, level);
  }
}

static void NaClValidatorPcAddressMess(
    int level,
    NaClValidatorState *state,
    NaClPcAddress addr,
    const char *format,
    va_list ap) {
  level = NaClRecordIfValidatorError(state, level);
  if (NaClPrintValidatorMessages(state, level)) {
    NaClPrintValidatorPrefix(level, state, !NACL_FLAGS_ncval_annotate);
    NaClPrintMessage(state, "%"NACL_PRIxNaClPcAddress ": ", addr);
    NaClPrintVMessage(state, format, ap);
    NaClRecordErrorReported(state, level);
  }
}

void NaClValidatorPcAddressMessage(int level,
                                   NaClValidatorState *state,
                                   NaClPcAddress addr,
                                   const char *format,
                                   ...) {
  va_list ap;
  va_start(ap, format);
  NaClValidatorPcAddressMess(level, state, addr, format, ap);
  va_end(ap);
}

void NaClValidatorInstMessage(int level,
                              NaClValidatorState *state,
                              NaClInstState *inst,
                              const char *format,
                              ...) {
  if (NACL_FLAGS_ncval_annotate) {
    va_list ap;
    va_start(ap, format);
    NaClValidatorPcAddressMess(level, state, inst->vpc, format, ap);
    va_end(ap);
  } else {
    level = NaClRecordIfValidatorError(state, level);
    if (NaClPrintValidatorMessages(state, level)) {
      va_list ap;
      NaClValidatorPrintInst(level, state, inst);
      NaClPrintValidatorPrefix(level, state, TRUE);
      va_start(ap, format);
      NaClPrintVMessage(state, format, ap);
      va_end(ap);
      NaClRecordErrorReported(state, level);
    }
  }
  if (state->do_stub_out && (level <= LOG_ERROR)) {
    NaClStubOutInst(inst);
  }
}

void NaClValidatorTwoInstMessage(int level,
                                 NaClValidatorState *state,
                                 NaClInstState *inst1,
                                 NaClInstState *inst2,
                                 const char *format,
                                 ...) {
  level = NaClRecordIfValidatorError(state, level);
  if (NaClPrintValidatorMessages(state, level)) {
    va_list ap;
    NaClPrintValidatorPrefix(level, state, TRUE);
    va_start(ap, format);
    NaClPrintVMessage(state, format, ap);
    va_end(ap);
    NaClPrintMessage(state, "\n                                   ");
    NaClValidatorPrintInst(level, state, inst1);
    NaClPrintMessage(state, "                                   ");
    NaClValidatorPrintInst(level, state, inst2);
    NaClRecordErrorReported(state, level);
  }
  if (state->do_stub_out && (level <= LOG_ERROR)) {
    NaClStubOutInst(inst2);
  }
}

Bool NaClValidatorQuit(NaClValidatorState *state) {
  return !state->validates_ok && (state->quit_after_error_count == 0);
}

void NaClRegisterValidator(
    NaClValidatorState *state,
    NaClValidator validator,
    NaClValidatorPostValidate post_validate,
    NaClValidatorPrintStats print_stats,
    NaClValidatorMemoryCreate create_memory,
    NaClValidatorMemoryDestroy destroy_memory) {
  NaClValidatorDefinition *defn;
  assert(NULL != validator);
  if (state->number_validators >= NACL_MAX_NCVALIDATORS) {
    NaClValidatorMessage(
        LOG_FATAL, state,
        "Too many validators specified, can't register. Quitting!\n");
  }
  defn = &state->validators[state->number_validators++];
  defn->validator = validator;
  defn->post_validate = post_validate;
  defn->print_stats = print_stats;
  defn->create_memory = create_memory;
  defn->destroy_memory = destroy_memory;
}

static void NaClNullErrorPrintInst(NaClErrorReporter* self,
                                   struct NaClInstState* inst) {}

NaClErrorReporter kNaClNullErrorReporter = {
  NaClNullErrorReporter,
  NaClNullErrorPrintf,
  NaClNullErrorPrintfV,
  (NaClPrintInst) NaClNullErrorPrintInst
};

NaClValidatorState *NaClValidatorStateCreate(const NaClPcAddress vbase,
                                             const NaClMemorySize sz,
                                             const uint8_t alignment,
                                             const NaClOpKind base_register) {
  NaClValidatorState *state;
  NaClValidatorState *return_value = NULL;
  NaClPcAddress vlimit = vbase + sz;
  DEBUG(NaClLog(LOG_INFO,
                "Validator Create: vbase = %"NACL_PRIxNaClPcAddress", "
                "sz = %"NACL_PRIxNaClMemorySize", alignment = %u, vlimit = %"
                NACL_PRIxNaClPcAddress"\n",
                vbase, sz, alignment, vlimit));
  if (vlimit <= vbase) return NULL;
  if (alignment != 16 && alignment != 32) return NULL;
  state = (NaClValidatorState*) malloc(sizeof(NaClValidatorState));
  if (state != NULL) {
    return_value = state;
    state->decoder_tables = kNaClValDecoderTables;
    state->vbase = vbase;
    state->alignment = alignment;
    state->vlimit = vlimit;
    state->alignment_mask = alignment - 1;
    if (NULL == nacl_validator_features) {
      NaClCPUData cpu_data;
      NaClCPUDataGet(&cpu_data);
      GetCPUFeatures(&cpu_data, &(state->cpu_features));
    } else {
      state->cpu_features = *nacl_validator_features;
    }
    state->base_register = base_register;
    state->validates_ok = TRUE;
    state->number_validators = 0;
    state->quit_after_error_count = NACL_FLAGS_max_reported_errors;
    state->error_reporter = &kNaClNullErrorReporter;
    state->print_opcode_histogram = NACL_FLAGS_opcode_histogram;
    state->trace_instructions = NACL_FLAGS_validator_trace_instructions;
    state->trace_inst_internals = NACL_FLAGS_validator_trace_inst_internals;
    state->log_verbosity = LOG_INFO;
    state->cur_inst_state = NULL;
    state->cur_inst = NULL;
    state->cur_inst_vector = NULL;
    state->quit = NaClValidatorQuit(return_value);
    state->do_stub_out = FALSE;
    state->rules_init_fn = NaClValidatorRulesInit;
  }
  return return_value;
}

/* Add validators to validator state if missing. Assumed to be called just
 * before analyzing a code segment.
 */
Bool NaClValidatorStateInitializeValidators(NaClValidatorState *state) {
  /* Note: Need to lazy initialize validators until after the call to
   * NaClValidateSegment, so that the user of the API can change flags.
   * Also, we must lazy initialize so that we don't break all callers,
   * who have followed the previous API.
   */
  Bool success = TRUE;  /* until proven otherwise. */
  if (0 == state->number_validators) {
    int i;
    (state->rules_init_fn)(state);
    for (i = 0; i < state->number_validators; ++i) {
      NaClValidatorDefinition *defn = &state->validators[i];
      if (defn->create_memory == NULL) {
        state->local_memory[i] = NULL;
      } else {
        void *defn_memory = defn->create_memory(state);
        if (defn_memory == NULL) {
          NaClValidatorMessage(
              LOG_ERROR, state,
              "Unable to create local memory for validator function!");
          state->local_memory[i] = NULL;
          success = FALSE;
        } else {
          state->local_memory[i] = defn_memory;
        }
      }
    }
  }
  return success;
}

void NaClValidatorStateCleanUpValidators(NaClValidatorState *state) {
  int i;
  for (i = 0; i < state->number_validators; ++i) {
    NaClValidatorDefinition *defn = &state->validators[i];
    void *defn_memory = state->local_memory[i];
    if (defn->destroy_memory != NULL && defn_memory != NULL) {
      defn->destroy_memory(state, defn_memory);
    }
  }
}

/* Given we are at the instruction defined by the instruction iterator, for
 * a segment, apply all applicable validator functions.
 */
static void NaClApplyValidators(NaClValidatorState *state, NaClInstIter *iter) {
  int i;
  DEBUG(NaClLog(LOG_INFO, "iter state:\n");
        NaClInstStateInstPrint(NaClLogGetGio(), NaClInstIterGetState(iter)));
  if (state->quit) return;
  for (i = 0; i < state->number_validators; ++i) {
    state->validators[i].validator(state, iter, state->local_memory[i]);
    if (state->quit) return;
  }
}

static void NaClRememberIpOnly(NaClValidatorState *state, NaClInstIter *iter) {
  void *local_memory;
  DEBUG(NaClLog(LOG_INFO, "iter state:\n");
        NaClInstStateInstPrint(NaClLogGetGio(), NaClInstIterGetState(iter)));
  if (state->quit) return;
  local_memory = NaClGetValidatorLocalMemory((NaClValidator) NaClJumpValidator,
                                             state);
  NaClJumpValidatorRememberIpOnly(state, iter, local_memory);
}

/* Given that we have just iterated through all instructions in a segment,
 * apply post validators rules (before we collect the iterator).
 */
static void NaClApplyPostValidators(NaClValidatorState *state,
                                    NaClInstIter *iter) {
  int i;
  DEBUG(NaClLog(LOG_INFO, "applying post validators...\n"));
  if (state->quit || (NULL == iter)) return;
  for (i = 0; i < state->number_validators; ++i) {
    if (NULL != state->validators[i].post_validate) {
      state->validators[i].post_validate(state, iter, state->local_memory[i]);
      if (state->quit) return;
    }
  }
}

static void NaClValidatorStatePrintStats(NaClValidatorState *state);

/* The maximum lookback for the instruction iterator of the segment. */
static const size_t kLookbackSize = 4;

void NaClValidateSegment(uint8_t *mbase, NaClPcAddress vbase,
                         NaClMemorySize size, NaClValidatorState *state) {
  NaClSegment segment;
  NaClInstIter *iter;
  if (NaClValidatorStateInitializeValidators(state)) {
    NCHaltTrimSegment(mbase, vbase, state->alignment, &size, &state->vlimit);
    NaClSegmentInitialize(mbase, vbase, size, &segment);
    do {
      iter = NaClInstIterCreateWithLookback(state->decoder_tables,
                                            &segment, kLookbackSize);
      if (NULL == iter) {
        NaClValidatorMessage(LOG_ERROR, state, "Not enough memory\n");
        break;
      }
      for (; NaClInstIterHasNextInline(iter); NaClInstIterAdvanceInline(iter)) {
        state->cur_inst_state = NaClInstIterGetStateInline(iter);
        state->cur_inst = NaClInstStateInst(state->cur_inst_state);
        state->cur_inst_vector = NaClInstStateExpVector(state->cur_inst_state);
        NaClApplyValidators(state, iter);
        if (state->quit) break;
      }
      state->cur_inst_state = NULL;
      state->cur_inst = NULL;
      state->cur_inst_vector = NULL;
    } while (0);
    NaClApplyPostValidators(state, iter);
    NaClInstIterDestroy(iter);
    NaClValidatorStatePrintStats(state);
  }
  NaClValidatorStateCleanUpValidators(state);
}

void NaClValidateSegmentUsingTables(uint8_t* mbase,
                                    NaClPcAddress vbase,
                                    NaClMemorySize sz,
                                    NaClValidatorState* state,
                                    const struct NaClDecodeTables* tables) {
  state->decoder_tables = tables;
  NaClValidateSegment(mbase, vbase, sz, state);
}

Bool NaClValidatesOk(NaClValidatorState *state) {
  return state->validates_ok;
}

static void NaClValidatorStatePrintStats(NaClValidatorState *state) {
  int i;
  for (i = 0; i < state->number_validators; i++) {
    NaClValidatorDefinition *defn = &state->validators[i];
    if (defn->print_stats != NULL) {
      defn->print_stats(state, state->local_memory[i]);
    }
  }
}

void NaClValidatorStateDestroy(NaClValidatorState *state) {
  free(state);
}

void *NaClGetValidatorLocalMemory(NaClValidator validator,
                                  const NaClValidatorState *state) {
  int i;
  for (i = 0; i < state->number_validators; ++i) {
    if (state->validators[i].validator == validator) {
      return state->local_memory[i];
    }
  }
  return NULL;
}

/*
 * Check that iter_new is a valid replacement for iter_old.
 * If a validation error occurs, state->validates_ok will be set to false by
 * NaClValidatorInstMessage when it is given LOG_ERROR, see the end of this
 * function.
 * Return value: TRUE if the instruction was changed, FALSE if it's identical.
 */
static Bool NaClValidateInstReplacement(NaClInstIter *iter_old,
                                        NaClInstIter *iter_new,
                                        struct NaClValidatorState *state) {
  NaClInstState *istate_old, *istate_new;
  NaClExpVector *exp_old, *exp_new;
  uint32_t i;
  Bool inst_changed = FALSE;

  istate_old = NaClInstIterGetStateInline(iter_old);
  istate_new = NaClInstIterGetStateInline(iter_new);

  /* Location/length must match */
  if (istate_new->vpc != istate_old->vpc ||
      istate_new->bytes.length != istate_old->bytes.length) {
    NaClValidatorTwoInstMessage(LOG_ERROR, state, istate_old, istate_new,
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
      if (exp_old->node[i].kind  != exp_new->node[i].kind) {
        goto error_exit;
      }
      if (exp_old->node[i].flags != exp_new->node[i].flags) {
        goto error_exit;
      }

      /*
       * allow some constants to be different; however it is important not to
       * allow modification of sandboxing instructions. Note neither of the
       * instructions allowed for modification is used for sandboxing
       */
      if (exp_old->node[i].value != exp_new->node[i].value) {
        if (exp_old->node[i].kind == ExprConstant) {

          /* allow different constants in direct calls */
          if (istate_old->inst->name == InstCall)
            if (exp_old->node[i].flags & NACL_EFLAG(ExprJumpTarget))
              if (exp_old->node[NaClGetExpParentIndex(exp_old, i)].kind
                    == OperandReference)
                continue;

          /*
           * allow different constants in operand of mov
           * e.g. mov $rax, 0xdeadbeef
           */
          if (istate_old->inst->name == InstMov)
            if (exp_old->node[i].flags & NACL_EFLAG(ExprUsed))
              if (exp_old->node[NaClGetExpParentIndex(exp_old, i)].kind
                    == OperandReference)
                continue;
          /*
           * allow different displacements in memory reference of mov
           * instructions e.g. mov $rax, [$r15+$rbx*2+0x7fff]
           */
          if (istate_old->inst->name == InstMov)
            if (exp_old->node[NaClGetExpParentIndex(exp_old, i)].kind
                    == ExprMemOffset)
              /* displacement is the fourth node after ExprMemOffset node */
              if (i - NaClGetExpParentIndex(exp_old, i) == 4)
                continue;
        }

        goto error_exit;
      }
    }

    /* This return signifies there is no error in validation. */
    return inst_changed;
  } while (0);

error_exit:
  /* This logging function is the mechanism that sets the validator state
   * to indicate an error.
   */
  NaClValidatorTwoInstMessage(LOG_ERROR, state, istate_old, istate_new,
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
                           struct NaClValidatorState *state) {
  NaClSegment segment_old, segment_new;
  NaClInstIter *iter_old = NULL;
  NaClInstIter *iter_new = NULL;

  NaClValidatorStateInitializeValidators(state);
  NaClSegmentInitialize(mbase_old, vbase, size, &segment_old);
  NaClSegmentInitialize(mbase_new, vbase, size, &segment_new);
  do {
    iter_old = NaClInstIterCreateWithLookback(state->decoder_tables,
                                              &segment_old, kLookbackSize);
    if (NULL == iter_old) break;
    iter_new = NaClInstIterCreateWithLookback(state->decoder_tables,
                                              &segment_new, kLookbackSize);
    if (NULL == iter_new) break;
    while (NaClInstIterHasNextInline(iter_old) &&
           NaClInstIterHasNextInline(iter_new)) {
      Bool inst_changed;
      state->cur_inst_state = NaClInstIterGetStateInline(iter_new);
      state->cur_inst = NaClInstStateInst(state->cur_inst_state);
      state->cur_inst_vector = NaClInstStateExpVector(state->cur_inst_state);
      inst_changed = NaClValidateInstReplacement(iter_old, iter_new, state);
      if (inst_changed)
        NaClApplyValidators(state, iter_new);
      else
        NaClRememberIpOnly(state, iter_new);
      if (state->quit) break;
      NaClInstIterAdvanceInline(iter_old);
      NaClInstIterAdvanceInline(iter_new);
    }
    if (NaClInstIterHasNextInline(iter_old) ||
        NaClInstIterHasNextInline(iter_new)) {
      NaClValidatorMessage(
          LOG_ERROR, state,
          "Code modification: code segments have different "
          "number of instructions\n");
    }
  } while (0);

  state->cur_inst_state = NULL;
  state->cur_inst = NULL;
  state->cur_inst_vector = NULL;
  NaClApplyPostValidators(state, iter_new);
  NaClInstIterDestroy(iter_old);
  NaClInstIterDestroy(iter_new);
  NaClValidatorStatePrintStats(state);
}
