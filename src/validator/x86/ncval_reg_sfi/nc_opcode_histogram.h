/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
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

#include "include/portability.h"
#include "src/utils/types.h"

/* Defines a validator state. */
struct NaClValidatorState;

/* Defines an instruction iterator that processes a code segment. */
struct NaClInstIter;

/* Defines a data structure that holds data defining the opcode histogram
 * being collected. Holds a histogram of the (first) byte of the found
 * opcodes for each instruction.
 */
typedef struct NaClOpcodeHistogram {
  uint32_t opcode_histogram[256];
} NaClOpcodeHistogram;

/* Command line flag controlling whether an opcode histogram is
 * collected while validating.
 */
extern Bool NACL_FLAGS_opcode_histogram;

/* Initializes opcode histogram data in the validator state. */
void NaClOpcodeHistogramInitialize(struct NaClValidatorState* state);

/* Validator function to record histgram value for current instruction. */
void NaClOpcodeHistogramRecord(struct NaClValidatorState* state);

/* Validator print function to print out collected histogram. */
void NaClOpcodeHistogramPrintStats(struct NaClValidatorState* state);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_NC_OPCODE_HISTOGRAPH_H__ */
