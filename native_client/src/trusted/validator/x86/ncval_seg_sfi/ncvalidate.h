/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_SEG_SFI_NCVALIDATE_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_SEG_SFI_NCVALIDATE_H__

#include <stdio.h>
#include "native_client/src/trusted/validator/x86/nacl_cpuid.h"
#include "native_client/src/trusted/validator/types_memory_model.h"

/*
 * ncvalidate.h: Validator for the segment-based sandbox.
 *
 * This is the primary library interface to the validator for the
 * segment-based sandbox. This version should be used when performance
 * is important. See ncvalidate_detailed.h for a secondary API which
 * provides more details when reporting errors.
 *
 * Basic usage:
 *   if (!NaClArchSuppported()) fail
 *   vstate = NCValidateInit(base, limit, 16)
 *   if vstate == 0 fail
 *   for each section:
 *     NCValidateSegment(maddr, vaddr, size, vstate);
 *   rc = NCValidateFinish();
 *   if rc != 0 fail
 *   NCValidateFreeState(&vstate);
 *
 * See the README file in this directory for more info on the general
 * structure of the validator.
 */
struct Gio;
struct NCDecoderInst;
struct NCValidatorState;
struct NaClErrorReporter;

/* NCValidateSetCPUFeatures: Define the set of CPU features to use.
 * Parameters:
 *    features: A pointer to a CPUFeatures to use, or NULL
 *       if the features set should be calculated using GetCPUFeatures.
 * Note: Assumes that given struct persists till the next call to
 *    this function. The main purpose of this function is to allow the injection
 *    of a command-line override of CPU features, from that of the local CPU id,
 *    for the tool ncval. As such, it uses a global variable to hold the command
 *    line specification in ncval. Also, we assume that this function is not
 *    called during a validation (i.e. between NCValidateInit and
 *    NCValidateFinish).
 */
void NCValidateSetCPUFeatures(CPUFeatures *features);

/*
 * Set the maximum number of diagnostic errors to be reported to the
 * given value (-1 implies all error messages).
 */
void NCValidateSetNumDiagnostics(struct NCValidatorState* vstate,
                                 int num_diagnostics);

/*
 * NCValidateInit: Initialize NaCl validator internal state.
 * Parameters:
 *    vbase: base virtual address for code segment
 *    vlimit: size in bytes of code segment
 *    alignment: 16 or 32, specifying alignment
 * Returns:
 *    an initialized struct NCValidatorState * if everything is okay,
 *    else NULL
 */
struct NCValidatorState *NCValidateInit(const NaClPcAddress vbase,
                                        const NaClPcAddress vlimit,
                                        const uint8_t alignment);

/*
 * Allows "stub out mode" to be enabled, in which some unsafe
 * instructions will be rendered safe by replacing them with HLT
 * instructions.
 */
void NCValidateSetStubOutMode(struct NCValidatorState *vstate,
                              int do_stub_out);

/*
 * Set the maximum number of diagnostic errors to be reported to the
 * given value (-1 implies all error messages).
 */
void NCValidateSetNumDiagnostics(struct NCValidatorState* vstate,
                                 int num_diagnostics);

/* Set the set of CPU features to use for this validation to
 * the given set of features.
 */
void NCValidatorStateSetCPUFeatures(struct NCValidatorState* vstate,
                                    const CPUFeatures* features);

/* Changes the error reporter to the given error reporter
 * for the given validator state.
 */
void NCValidateSetErrorReporter(struct NCValidatorState* vstate,
                                struct NaClErrorReporter* error_reporter);

/* Validate a segment */
/* This routine will raise an segmentation exception if you ask
 * it to check memory that can't be accessed. This should of be
 * interpreted as an indication that the module in question is
 * invalid.
 */
void NCValidateSegment(uint8_t *mbase, NaClPcAddress vbase,
                       NaClMemorySize sz,
                       struct NCValidatorState *vstate);

/* Validate a segment for dynamic code replacement */
/* This routine checks that the code found at mbase_old
 * can be dynamically replaced with the code at mbase_new
 * safely. Returns non-zero if successful.
 */
int NCValidateSegmentPair(uint8_t *mbase_old, uint8_t *mbase_new,
                          NaClPcAddress vbase, size_t sz,
                          uint8_t alignment);

/* Check targets and alignment. Returns non-zero if there are */
/* safety issues, else returns 1                              */
/* BEWARE: vstate is invalid after this call                  */
int NCValidateFinish(struct NCValidatorState *vstate);

/* BEWARE: this call deallocates vstate.                      */
void NCValidateFreeState(struct NCValidatorState **vstate);

/* Print some interesting statistics... (optional). If used,
 * should be called between NCValidateFinish and
 * NCValidateFreeState.
 *
 * Note: Uses error reporter of validator to print messages.
 * The default error reporter of the validator will not
 * print any messages. To actually get the messages, you
 * must associate an error reporter with the validator using
 * NCValidateSetErrorReporter.
 */
void NCStatsPrint(struct NCValidatorState *vstate);

/* Returns the default value used for controlling printing
 * of validator messages.
 * If zero, no messages are printed.
 * If >0, only that many diagnostic errors are printed.
 * If negative, all validator diagnostics are printed.
 */
int NCValidatorGetMaxDiagnostics();

/* Changes default flag for printing validator error messages.
 * If zero, no messages are printed.
 * If >0, only that many diagnostic errors are printed.
 * If negative, all validator diagnostics are printed.
 */
void NCValidatorSetMaxDiagnostics(int new_value);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_SEG_SFI_NCVALIDATE_H__ */
