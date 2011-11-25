/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVALIDATE_ERROR_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVALIDATE_ERROR_H__

/*
 * Defines an API for reporting error messages by the validator. The
 * API is a struct of function pointers (i.e. virtuals) which will
 * be called by the validator to print out error messages.
 *
 * The typical error message is generated using the form:
 *
 * error_reporter->printf(format, ...)
 *
 * Note: Levels are assumed to be defined by the constants
 * LOG_INFO, LOG_WARNING, LOG_ERROR, and LOG_FATAL defined
 * by header file native_client/src/shared/platform/nacl_log.h"
 */

#include <stdarg.h>
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

struct NaClErrorReporter;

/* Enumeration defining type of expected error reporter, based on
 * model of instructions to be printed.
 */
typedef enum NaClErrorReporterSupported {
  /* The following defines the null error reporter, which reports
   * no errors, and therefore, can support any model of instructions
   * to be printed.
   */
  NaClNullErrorReporter,

  /* The following defines that the error reporter handles calls
   * to NaClPrintInst using a NaClInstState* instruction argument.
   */
  NaClInstStateErrorReporter,

  /* The following defines that the error reporter handles calls
   * to NaClPrintInst using a NCDecoderInst* instruction argument.
   */
  NCDecoderInstErrorReporter
} NaClErrorReporterSupported;

/* Returns the name of the NaClErrorReporterSupported value. */
const char* NaClErrorReporterSupportedName(NaClErrorReporterSupported kind);

/* Method to print out a formatted string. */
typedef void (*NaClPrintfMessage)(
    struct NaClErrorReporter* self, const char* format, ...);

/* Method to print out a formatted string. */
typedef void (*NaClPrintfVMessage)(struct NaClErrorReporter* self,
                                   const char* format,
                                   va_list ap);

/* Method to print out a parsed instruction.
 * WARNING: the inst parameter is either a NaClInstState* (if the
 * iterator parser is being used), or NCDecoderInst* (if the callback
 * parser is being used). Be sure to use the correct error reporter
 * for the parser being used.
 */
typedef void (*NaClPrintInst)(struct NaClErrorReporter* self, void* inst);

/* The virtual (base) class of virtual printing methods. */
typedef struct NaClErrorReporter {
  NaClErrorReporterSupported supported_reporter;
  NaClPrintfMessage  printf;
  NaClPrintfVMessage printf_v;
  NaClPrintInst      print_inst;
} NaClErrorReporter;

/* Default null printf function for error reporter. */
void NaClNullErrorPrintf(NaClErrorReporter* self,
                         const char* format, ...);

/* Default null printf_v function for error reporter. */
void NaClNullErrorPrintfV(NaClErrorReporter* self,
                          const char* format,
                          va_list ap);

/* Default verbose printf function that reports to NaClLogGetGio(). */
void NaClVerboseErrorPrintf(NaClErrorReporter* self,
                            const char* format, ...);

/* Default verbose printf_v function that reports to NaClLogGetGio(). */
void NaClVerboseErrorPrintfV(NaClErrorReporter* self,
                             const char* format,
                             va_list ap);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVALIDATE_ERROR_H__ */
