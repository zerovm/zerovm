/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
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
 *     NaClValidatorStateCreate(base, limit - base, 32, RegR15);
 *   if (state == NULL) fail;
 *   for each section:
 *     NaClValidateSegment(maddr, vaddr, size, state);
 *   if (!NaClValidatesOk(state)) fail;
 *   NaClValidatorStateDestroy(state);
 */

#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/validator/types_memory_model.h"
#include "native_client/src/trusted/validator/x86/decoder/gen/ncopcode_operand_kind.h"
#include "native_client/src/trusted/validator/x86/error_reporter.h"
#include "native_client/src/trusted/validator/x86/nacl_cpuid.h"

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

/* Define the stop instruction. */
extern const uint8_t kNaClFullStop;

/* Changes all validator trace flags to true. */
void NaClValidatorFlagsSetTraceVerbose();

/* The model of a validator state. */
typedef struct NaClValidatorState NaClValidatorState;

/* NaClValidateSetCPUFeatures: Define the set of CPU features to use.
 * Parameters:
 *    features: A pointer to a CPUFeatures to use, or NULL
 *       if the features set should be calculated using GetCPUFeatures.
 * Note: Assumes that given struct persists till the next call to
 *    this function. The main purpose of this function is to allow the
 *    injection of a command-line override of CPU features, from that of the
 *    local CPU id, for the tool ncval. As such, it uses a global variable to
 *    hold the command line specification in ncval. Also, we assume that this
 *    function is not called during validation (i.e. between
 *    NaClValidatorStateCreate and NaClValidatorStateDestroy).
 */
void NaClValidateSetCPUFeatures(CPUFeatures *features);

/* Create a validator state to validate the code segment.
 * Note: Messages (if any) produced by the validator are sent to the stream
 * defined by native_client/src/shared/platform/nacl_log.h.
 * Parameters.
 *   vbase - The virtual address for the contents of the code segment.
 *   sz - The number of bytes in the code segment.
 *   alignment: 16 or 32, specifying alignment.
 *   base_register - OperandKind defining value for base register (or
 *     RegUnknown if not defined).
 * Returns:
 *   A pointer to an initialized validator state if everything is ok, NULL
 *  otherwise.
 */
NaClValidatorState* NaClValidatorStateCreate(const NaClPcAddress vbase,
                                             const NaClMemorySize sz,
                                             const uint8_t alignment,
                                             const NaClOpKind base_register);

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
 */
void NaClValidatorStateSetMaxReportedErrors(NaClValidatorState* state,
                                            int max_reported_errors);

/* Changes the set of cpu features to use to the given featers. */
void NaClValidatorStateSetCPUFeatures(NaClValidatorState* state,
                                     const CPUFeatures* features);

/* Changes the report error reported for the validator. By default, no
 * error messages are printed. To print error messages, use an appropriate
 * error printer, such as  kNaClVerboseErrorReporter in ncval_driver.h.
 * Note: Should only be called between calls to NaClValidatorStateCreate
 * and NaClValidateSegment. If not set, the validator will not print
 * error messages.
 */
void NaClValidatorStateSetErrorReporter(NaClValidatorState* state,
                                        struct NaClErrorReporter* reporter);

/* A default, null error reporter for a NCInstState* */
extern NaClErrorReporter kNaClNullErrorReporter;

/* Verbose error reporter for a NaClInstState* that reports to
 * NaClLogGetGio().
 */
extern NaClErrorReporter kNaClVerboseErrorReporter;

/* Returns true if an opcode histogram should be printed by the validator.
 * Note: Defaults to NACL_FLAGS_opcode_histogram.
 */
Bool NaClValidatorStateGetPrintOpcodeHistogram(NaClValidatorState* state);

/* Changes the value on whether the opcode histogram should be printed by
 * the validator.
 * Note: Should only be called between calls to NaClValidatorStateCreate
 * and NaClValidateSegment.
 */
void NaClValidatorStateSetPrintOpcodeHistogram(NaClValidatorState* state,
                                               Bool new_value);

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

/* Defines a function to create local memory to be used by a validator
 * function, should it need it.
 * Parameters:
 *   state - The state of the validator.
 * Returns:
 *   Allocated space for local data associated with a validator function.
 */
typedef void* (*NaClValidatorMemoryCreate)(NaClValidatorState* state);

/* Defines a validator function to be called on each instruction.
 * Parameters:
 *   state - The state of the validator.
 *   iter - The instruction iterator's current position in the segment.
 *   local_memory - Pointer to local memory generated by the corresponding
 *          NaClValidatorMemoryCreate (or NULL if not specified).
 */
typedef void (*NaClValidator)(NaClValidatorState* state,
                              struct NaClInstIter* iter,
                              void* local_memory);

/* Defines a post validator function that is called after iterating through
 * a segment, but before the iterator is destroyed.
 * Parameters:
 *   state - The state of the validator,
 *   iter - The instruction iterator's current position in the segment.
 *   local_memory - Pointer to local memory generated by the corresponding
 *          NaClValidatorMemoryCreate (or NULL if not specified).
 */
typedef void (*NaClValidatorPostValidate)(NaClValidatorState* state,
                                          struct NaClInstIter* iter,
                                          void* local_memory);

/* Defines a statistics print routine for a validator function.
 * Note: statistics will be printed to (nacl) log file.
 * Parameters:
 *   state - The state of the validator,
 *   local_memory - Pointer to local memory generated by the corresponding
 *          NaClValidatorMemoryCreate (or NULL if not specified).
 */
typedef void (*NaClValidatorPrintStats)(NaClValidatorState* state,
                                        void* local_memory);

/* Defines a function to destroy local memory used by a validator function,
 * should it need to do so.
 * Parameters:
 *   state - The state of the validator.
 *   local_memory - Pointer to local memory generated by the corresponding
 *         NaClValidatorMemoryCreate (or NULL if not specified).
 */
typedef void (*NaClValidatorMemoryDestroy)(NaClValidatorState* state,
                                           void* local_memory);

/* Registers a validator function to be called during validation.
 * Parameters are:
 *   state - The state to register the validator functions in.
 *   validator - The validator function to register.
 *   post_validate - Validate global context after iterator run.
 *   print_stats - The print function to print statistics about the applied
 *     validator.
 *   memory_create - The function to call to generate local memory for
 *     the validator function (or NULL if no local memory is needed).
 *   memory_destroy - The function to call to reclaim local memory when
 *     the validator state is destroyed (or NULL if reclamation is not needed).
 */
void NaClRegisterValidator(NaClValidatorState* state,
                           NaClValidator validator,
                           NaClValidatorPostValidate post_validate,
                           NaClValidatorPrintStats print_stats,
                           NaClValidatorMemoryCreate memory_create,
                           NaClValidatorMemoryDestroy memory_destroy);

/* Returns the local memory associated with the given validator function,
 * or NULL if no such memory exists. Allows validators to communicate
 * shared collected information.
 * Parameters:
 *   validator - The validator function's memory you want access to.
 *   state - The current state of the validator.
 * Returns:
 *   The local memory associated with the validator (or NULL  if no such
 *   validator is known).
 */
void* NaClGetValidatorLocalMemory(NaClValidator validator,
                                  const NaClValidatorState* state);

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

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NCVALIDATE_ITER_H__ */
