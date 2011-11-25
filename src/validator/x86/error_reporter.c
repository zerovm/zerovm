/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/validator/x86/error_reporter.h"

#include "include/nacl_macros.h"
#include "src/gio/gio.h"
#include "src/platform/nacl_log.h"

static const char* NaClErrorReporterSupportedNames[] = {
  "NaClNullErrorReporter",
  "NaClInstStateErrorReporter",
  "NCDecoderInstErrorReporter"
};

/* Note: We can't move NaClErrorReporterSupportedName to
 * error_reporter_verbose.c because ncval_seg_sfi/ncdecode.c needs
 * to use it, and it is part of the base libraries needed by sel_ldr.
 */
const char* NaClErrorReporterSupportedName(NaClErrorReporterSupported kind) {
  if ((0 < kind) &&
      (kind < NACL_ARRAY_SIZE(NaClErrorReporterSupportedNames))) {
    return NaClErrorReporterSupportedNames[kind];
  }
  return "NCDecoderInstErrorReporter";
}

void NaClNullErrorPrintf(NaClErrorReporter* self,
                                const char* format, ...) {}

void NaClNullErrorPrintfV(NaClErrorReporter* self,
                          const char* format,
                          va_list ap) {}
