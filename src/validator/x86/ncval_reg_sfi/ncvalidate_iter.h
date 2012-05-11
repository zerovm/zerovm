/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATE_ITER_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATE_ITER_H__

/*
 * ncvalidate_iter.h: Validator for the register-based SFI sandbox.
 *
 * This is the primary library interface to the x86-64 validator for the
 * register-based sandbox. This version should be used when performance
 * is important. See ncvalidate_iter_detailed.h for a secondary API which
 * provides more details when reporting errors.
 *
 * Basic usage:
 *   -- base is initial address of ELF file.
 *   -- limit is the size of the ELF file.
 *   -- maddr is the address to the memory of a section.
 *   -- vaddr is the starting virtual address associated with a section.
 *   -- size is the number of bytes in a section.
 *
 *   if (!NaClArchSupported()) fail;
 *   NaClValidatorState* state =
 *     NaClValidatorStateCreate(base, limit - base, 32, readonly, RegR15);
 *   if (state == NULL) fail;
 *   for each section:
 *     NaClValidateSegment(maddr, vaddr, size, state);
 *   if (!NaClValidatesOk(state)) fail;
 *   NaClValidatorStateDestroy(state);
 */

#include "include/portability.h"
#include "src/utils/types.h"
#include "src/validator/types_memory_model.h"
#include "src/validator/x86/decoder/gen/ncopcode_operand_kind.h"
#include "src/validator/x86/error_reporter.h"
#include "src/validator/x86/nacl_cpuid.h"

EXTERN_C_BEGIN

struct NaClDecodeTables;
struct NaClInstIter;
struct NaClInstState;
struct NaClValidatorState;

/* Control flag that when set to FALSE, turns of the printing of validator
 * messages.
 */
extern Bool NACL_FLAGS_print_validator_messages;

/* When >= 0, only print this many errors before quiting. When
 * < 0, print all errors.
 */
extern int NACL_FLAGS_max_reported_errors;

/* Command line flag controlling whether each instruction is traced
 * while validating instructions.
 */
extern Bool NACL_FLAGS_validator_trace_instructions;

/* Command line flag controlling whether the internal representation
 * of each instruction is trace while validating.
 * Command line flag controlling whether each instruction, and its
 * corresponding internal details, is traced while validating
 * instructions.
 */
extern Bool NACL_FLAGS_validator_trace_inst_interals;

/* Command line flag controlling whether address error messages
 * should be printed out using the format needed by ncval_annotate.py
 */
extern Bool NACL_FLAGS_ncval_annotate;

#ifdef NCVAL_TESTING
/* Command line flag for printing out prefix/postfix conditions. */
extern Bool NACL_FLAGS_print_validator_conditions;

/* Command line flag controlling whether pre/post conditions are printed
 * on all instructions. By default, only those instructions that validate
 * have pre/post conditions printed.
 */
extern Bool NACL_FLAGS_report_conditions_on_all;
#endif

/* Changes all validator trace flags to true. */
void NaClValidatorFlagsSetTraceVerbose();

/* The model of a validator state. */
typedef struct NaClValidatorState NaClValidatorState;

/* Create a validator state to validate the code segment.
 * Note: Messages (if any) produced by the validator are sent to the stream
 * defined by src/platform/nacl_log.h.
 * Parameters.
 *   vbase - The virtual address for the contents of the code segment.
 *   sz - The number of bytes in the code segment.
 *   base_register - OperandKind defining value for base register (or
 *     RegUnknown if not defined).
 *   readonly - Whether the text should be treated as read-only.
 *   features - The CPU features to use. Uses local features of machine if NULL.
 * Returns:
 *   A pointer to an initialized validator state if everything is ok, NULL
 *  otherwise.
 */
NaClValidatorState* NaClValidatorStateCreate(
    const NaClPcAddress vbase,
    const NaClMemorySize codesize,
    const NaClOpKind base_register,
    const int readonly, /* Bool */
    const NaClCPUFeaturesX86 *features);

/* Returns true if the instruction iterator of the validator has any more
 * instructions. Also does any necessary internal caching if there are
 * more instructions, based on the instruction iterator.
 */
Bool NaClValidatorStateIterHasNext(NaClValidatorState *vstate);

/* Advances the instruction iterator of the validator to the next instruction.
 * Also does necessary internal caching expected by the validator for the
 * next instruction.
 */
void NaClValidatorStateIterAdvance(NaClValidatorState *vstate);

/* Assumes that we have finished iterating through the instruction iterator
 * of the validator and cleans up appropriate cached information that is
 * only defined while iterating over instructions.
 */
void NaClValidatorStateIterFinish(NaClValidatorState *vstate);

/* Resets the instruction iterator back to the beginning of the segment.
 * Returns true on success.
 */
Bool NaClValidatorStateIterReset(NaClValidatorState *vstate);

/* Returns the current maximum number of errors that can be reported.
 * Note: When > 0, the validator will only print that many errors before
 * quiting. When 0, the validator will not print any messages. When < 0,
 * the validator will print all found errors.
 * Note: Defaults to NACL_FLAGS_max_reported_errors.
 */
int NaClValidatorStateGetMaxReportedErrors(NaClValidatorState* state);

/* Changes the current maximum number of errors that will be reported before
 * quiting. For legal parameter values, see
 * NaClValidatorStateGetMaxReportedErrors.
 * Note: Should only be called between calls to NaClValidatorStateCreate
 * and NaClValidateSegment.
 * Note: This function will have no effect unless
 * NaClValidatorStateSetErrorReporter is called to define error reporting.
 */
void NaClValidatorStateSetMaxReportedErrors(NaClValidatorState* state,
                                            int max_reported_errors);

/* Changes the report error reported for the validator. By default, no
 * error messages are printed. To print error messages, use an appropriate
 * error printer, such as  kNaClVerboseErrorReporter in ncval_driver.h.
 * Note: Should only be called between calls to NaClValidatorStateCreate
 * and NaClValidateSegment. If not set, the validator will not print
 * error messages.
 * Note: Even if the error reporter is set to kNaClVerboseErrorReporter,
 * errors will not be reported unless you also change the maximum number
 * of errors reported via a call to NaClValidatorStateSetMaxReportedErrors.
 * The reason for this is that the default number of reported errors is zero
 * (based on the default use of the validator in sel_ldr).
 */
void NaClValidatorStateSetErrorReporter(NaClValidatorState* state,
                                        struct NaClErrorReporter* reporter);

/* A default, null error reporter for a NCInstState* */
extern NaClErrorReporter kNaClNullErrorReporter;

/* Verbose error reporter for a NaClInstState* that reports to
 * NaClLogGetGio().
 */
extern NaClErrorReporter kNaClVerboseErrorReporter;

/* Returns true if each instruction should be printed as the validator
 * processes the instruction.
 * Note: Defaults to NACL_FLAGS_validator_trace.
 */
Bool NaClValidatorStateGetTraceInstructions(NaClValidatorState* state);

/* Changes the value on whether each instruction should be printed as
 * the validator processes the instruction.
 * Note: Should only be called between calls to NaClValidatorStateCreate
 * and NaClValidateSegment.
 */
void NaClValidatorStateSetTraceInstructions(NaClValidatorState* state,
                                            Bool new_value);

/* Returns true if the internal representation of each instruction
 * should be printed as the validator processes the instruction.
 * Note: Should only be called between calls to NaClValidatorStateCreate
 * and NaClValidateSegment.
 */
Bool NaClValidatorStateGetTraceInstInternals(NaClValidatorState* state);

/* Changes the value on whether the internal details of each validated
 * instruction should be printed, as the validator visits the instruction.
 * Note: Should only be called between calls to NaClValidatorStateCreate
 * and NaClValidateSegment.
 */
void NaClValidatorStateSetTraceInstInternals(NaClValidatorState* state,
                                             Bool new_value);

/* Returns true if any of thevalidator trace flags are set.
 * Note: If this function returns true, so does
 *    NaClValidatorStateGetTraceInstructions
 *    NaClValidatorStateGetTraceInstInternals
 */
Bool NaClValidatorStateTrace(NaClValidatorState* state);

/* Convenience function that changes all validator trace flags to true.
 * Note: Should only be called between calls to NaClValidatorStateCreate
 * and NaClValidateSegment.
 */
void NaClValidatorStateSetTraceVerbose(NaClValidatorState* state);

/* Returns the log verbosity for printed validator messages. Legal
 * values are defined by src/shared/platform/nacl_log.h.
 * Note: Defaults to LOG_INFO.
 */
int NaClValidatorStateGetLogVerbosity(NaClValidatorState* state);

/* Changes the log verbosity for printed validator messages to the
 * new value. Legal values are defined by src/shared/platform/nacl_log.h.
 * Note: Should only be called between calls to NaClValidatorStateCreate
 * and NaClValidateSegment.
 * Note: NaClLogGetVerbosity() can override this value if more severe
 * than the value defined here. This allows a global limit (defined
 * by nacl_log.h) as well as a validator specific limit.
 */
void NaClValidatorStateSetLogVerbosity(NaClValidatorState* state,
                                       Bool new_value);

/* Return the value of the "do stub out" flag, i.e. whether instructions will
 * be stubbed out with HLT if they are found to be illegal.
 */
Bool NaClValidatorStateGetDoStubOut(NaClValidatorState* state);

/* Changes the "do stub out" flag to the given value. Note: Should only
 * be called between calls to NaClValidatorStateCreate and NaClValidateSegment.
 */
void NaClValidatorStateSetDoStubOut(NaClValidatorState* state,
                                    Bool new_value);

/* Stub out "num" bytes starting at "ptr". */
void NCStubOutMem(NaClValidatorState *state, void *ptr, size_t num);

/* Validate a code segment.
 * Parameters:
 *   mbase - The address of the beginning of the code segment.
 *   vbase - The virtual address associated with the beginning of the code
 *       segment.
 *   sz - The number of bytes in the code segment.
 *   state - The validator state to use while validating.
 */
void NaClValidateSegment(uint8_t* mbase,
                         NaClPcAddress vbase,
                         NaClMemorySize sz,
                         NaClValidatorState* state);

/* Same as NaClValidateSegment, except that the given decoder table is used
 * instead.
 */
void NaClValidateSegmentUsingTables(uint8_t* mbase,
                                    NaClPcAddress vbase,
                                    NaClMemorySize sz,
                                    NaClValidatorState* state,
                                    const struct NaClDecodeTables* tables);

/*
 * Validate a segment for dynamic code replacement
 * Checks if code at mbase_old can be replaced with code at mbase_new
 * Note that mbase_old was validated when it was inserted originally.
 * If validation fails, state->validates_ok will be set to false.
 * Parameters:
 *    mbase_old - The address of the beginning of the code segment to be
 *                replaced
 *    mbase_new - The address of the code segment that replaces the old
 *                segment
 *    vbase     - Virtual address that is associated with both segments
 *    size      - Length of the code segments (the segments must be of the same
 *                size)
 *    state     - The validator state to use while validating *new* segment
 */
void NaClValidateSegmentPair(uint8_t *mbase_old,
                             uint8_t *mbase_new,
                             NaClPcAddress vbase,
                             size_t size,
                             struct NaClValidatorState *state);

/* Returns true if the validator hasn't found any problems with the validated
 * code segments.
 * Parameters:
 *   state - The validator state used to validate code segments.
 * Returns:
 *   true only if no problems have been found.
 */
Bool NaClValidatesOk(NaClValidatorState* state);

/* Cleans up and returns the memory created by the corresponding
 * call to NaClValidatorStateCreate.
 */
void NaClValidatorStateDestroy(NaClValidatorState* state);

/* Prints out a validator message for the given level.
 * Parameters:
 *   level - The level of the message, as defined in nacl_log.h
 *   state - The validator state that detected the error.
 *   format - The format string of the message to print.
 *   ... - arguments to the format string.
 */
void NaClValidatorMessage(int level,
                          NaClValidatorState* state,
                          const char* format,
                          ...) ATTRIBUTE_FORMAT_PRINTF(3, 4);

/* Prints out a validator message for the given level using
 * a variable argument list.
 * Parameters:
 *   level - The level of the message, as defined in nacl_log.h
 *   state - The validator state that detected the error.
 *   format - The format string of the message to print.
 *   ap - variable argument list for the format.
 */
void NaClValidatorVarargMessage(int level,
                                NaClValidatorState* state,
                                const char* format,
                                va_list ap);

/* Prints out a validator message for the given address.
 * Parameters:
 *   level - The level of the message, as defined in nacl_log.h
 *   state - The validator state that detected the error.
 *   addr - The address where the error occurred.
 *   format - The format string of the message to print.
 *   ... - arguments to the format string.
 */
void NaClValidatorPcAddressMessage(int level,
                                   NaClValidatorState* state,
                                   NaClPcAddress addr,
                                   const char* format,
                                   ...) ATTRIBUTE_FORMAT_PRINTF(4, 5);

/* Prints out a validator message for the given instruction.
 * Parameters:
 *   level - The level of the message, as defined in nacl_log.h
 *   state - The validator state that detected the error.
 *   inst - The instruction that caused the vaidator error.
 *   format - The format string of the message to print.
 *   ... - arguments to the format string.
 */
void NaClValidatorInstMessage(int level,
                              NaClValidatorState* state,
                              struct NaClInstState* inst,
                              const char* format,
                              ...) ATTRIBUTE_FORMAT_PRINTF(4, 5);

/* Prints out a validator message and two given instructions.
 * Parameters:
 *   level - The level of the message, as defined in nacl_log.h
 *   state - The validator state that detected the error.
 *   inst1 - The first instruction to be printed.
 *   inst2 - The second instruction to be printed.
 *   format - The format string of the message to print.
 *   ... - arguments to the format string.
 */
void NaClValidatorTwoInstMessage(int level,
                                 NaClValidatorState* state,
                                 struct NaClInstState* inst1,
                                 struct NaClInstState* inst2,
                                 const char* format,
                                 ...) ATTRIBUTE_FORMAT_PRINTF(5, 6);

/* Returns true if the validator should quit due to previous errors. */
Bool NaClValidatorQuit(NaClValidatorState* state);

/* Returns true if any code has been overwritten with halts. */
Bool NaClValidatorDidStubOut(NaClValidatorState *vstate);

#ifdef NCVAL_TESTING
/* Defines the buffer and the corresponding buffer size to use for SNPRINTF,
 * given the current contents of the pre/post condition.
 */
void NaClConditionAppend(char* condition,
                         char** buffer,
                         size_t* buffer_size);

/* Prints out the address of the current instruction, and the pre/post
 * conditions associated with the current instruction.
 */
void NaClPrintConditions(NaClValidatorState *state);
#endif

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATE_ITER_H__ */
