/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * Collects histogram information as a validator function.
 *
 * Note: The following functions are used to define a validator function
 * for collecting this information. See header file ncvalidator_iter.h
 * for more information on how to register these functions as a validator
 * function.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_OPCODE_HISTOGRAPH_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_OPCODE_HISTOGRAPH_H__

/* Defines a validator state. */
struct NaClValidatorState;

/* Defines an instruction iterator that processes a code segment. */
struct NaClInstIter;

/* Defines a data structure that holds data defining the opcode histogram
 * being collected.
 */
struct NaClOpcodeHistogram;

/* Creates memory to hold an opcode histogram. */
struct NaClOpcodeHistogram* NaClOpcodeHistogramMemoryCreate(
    struct NaClValidatorState* state);

/* Destroys memory holding an opcode histogram. */
void NaClOpcodeHistogramMemoryDestroy(struct NaClValidatorState* state,
                                      struct NaClOpcodeHistogram* histogram);

/* Validator function to record histgram value for current instruction
 * in instruction iterator.
 */
void NaClOpcodeHistogramRecord(struct NaClValidatorState* state,
                               struct NaClInstIter* iter,
                               struct NaClOpcodeHistogram* histogram);

/* Validator print function to print out collected histogram. */
void NaClOpcodeHistogramPrintStats(struct NaClValidatorState* state,
                                   struct NaClOpcodeHistogram* histogram);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_OPCODE_HISTOGRAPH_H__ */
