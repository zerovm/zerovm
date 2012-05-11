/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/platform/nacl_log.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/validator/ncvalidate.h"

const size_t kMinimumCachedCodeSize = 40000;

/* Translate validation status to values wanted by sel_ldr. */
static int NaClValidateStatus(NaClValidationStatus status) {
  switch (status) {
    case NaClValidationSucceeded:
      return LOAD_OK;
    case NaClValidationFailedOutOfMemory:
      /* Note: this is confusing, but is what sel_ldr is expecting. */
      return LOAD_BAD_FILE;
    case NaClValidationFailed:
    case NaClValidationFailedNotImplemented:
    case NaClValidationFailedCpuNotSupported:
    case NaClValidationFailedSegmentationIssue:
    default:
      return LOAD_VALIDATION_FAILED;
  }
}

typedef NaClValidationStatus (*ValidateFunc) (
    uintptr_t, uint8_t*, size_t, int, int,
    const NaClCPUFeatures*, struct NaClValidationCache*);

static ValidateFunc NaClSelectValidator(struct NaClApp *nap) {
  ValidateFunc ret = NACL_SUBARCH_NAME(ApplyValidator,
                                       NACL_TARGET_ARCH, NACL_TARGET_SUBARCH);
  /* Avoid linking two validators into Chromium to keep download size small. */
//#if defined(__arm__) || !defined(NACL_STANDALONE)
//  UNREFERENCED_PARAMETER(nap);
//#else
//  if (nap->enable_dfa_validator) {
//    ret = NACL_SUBARCH_NAME(ApplyDfaValidator,
//                            NACL_TARGET_ARCH, NACL_TARGET_SUBARCH);
//  }
//#endif
  return ret;
}

int NaClValidateCode(struct NaClApp *nap, uintptr_t guest_addr,
                     uint8_t *data, size_t size) {
  NaClValidationStatus status = NaClValidationSucceeded;
  struct NaClValidationCache *cache = NULL;
  ValidateFunc validate_func = NaClSelectValidator(nap);

  if (size < kMinimumCachedCodeSize) {
    /*
     * Don't cache the validation of small code chunks for three reasons:
     * 1) The size of the validation cache will be bounded.  Cache entries are
     *    better used for bigger code.
     * 2) The per-transaction overhead of validation caching is more noticeable
     *    for small code.
     * 3) JITs tend to generate a lot of small code chunks, and JITed code may
     *    never be seen again.  Currently code size is the best mechanism we
     *    have for heuristically distinguishing between JIT and static code.
     *    (In practice most Mono JIT blocks are less than 1k, and a quick look
     *    didn't show any above 35k.)
     * The choice of what constitutes "small" is arbitrary, and should be
     * empirically tuned.
     * TODO(ncbray) let the syscall specify if the code is cached or not.
     */
    cache = NULL;
  }

  if (nap->validator_stub_out_mode) {
    /* Validation caching is currently incompatible with stubout. */
    cache = NULL;
    /* In stub out mode, we do two passes.  The second pass acts as a
       sanity check that bad instructions were indeed overwritten with
       allowable HLTs. */
    status = validate_func(guest_addr, data, size,
                           TRUE, /* stub out */
                           FALSE, /* text is not read-only */
                           &nap->cpu_features,
                           cache);
  }
  if (status == NaClValidationSucceeded) {
    /* Fixed feature CPU mode implies read-only. */
    int readonly_text = nap->fixed_feature_cpu_mode;
    status = validate_func(guest_addr, data, size,
                           FALSE, /* do not stub out */
                           readonly_text,
                           &nap->cpu_features,
                           cache);
  }
  return NaClValidateStatus(status);
}

int NaClValidateCodeReplacement(struct NaClApp *nap, uintptr_t guest_addr,
                                uint8_t *data_old, uint8_t *data_new,
                                size_t size) {
  if (nap->validator_stub_out_mode) return LOAD_BAD_FILE;
  if (nap->fixed_feature_cpu_mode) return LOAD_BAD_FILE;

  if ((guest_addr % nap->bundle_size) != 0 ||
      (size % nap->bundle_size) != 0) {
    return LOAD_BAD_FILE;
  }

  return NaClValidateStatus(
      NACL_SUBARCH_NAME(ApplyValidatorCodeReplacement,
                        NACL_TARGET_ARCH,
                        NACL_TARGET_SUBARCH)
      (guest_addr, data_old, data_new, size, &nap->cpu_features));
}

int NaClCopyCode(struct NaClApp *nap, uintptr_t guest_addr,
                 uint8_t *data_old, uint8_t *data_new,
                 size_t size) {
  /* Fixed-feature mode disables any code copying for now. Currently
   * the only use of NaClCodeCopy() seems to be for dynamic code
   * modification, which should fail in NaClValidateCodeReplacement()
   * before reaching this.
   */
  if (nap->fixed_feature_cpu_mode) return LOAD_BAD_FILE;
  return NaClValidateStatus(
      NACL_SUBARCH_NAME(ApplyValidatorCopy,
                        NACL_TARGET_ARCH,
                        NACL_TARGET_SUBARCH)
      (guest_addr, data_old, data_new, size, &nap->cpu_features));
}

NaClErrorCode NaClValidateImage(struct NaClApp  *nap) {
  uintptr_t               memp;
  uintptr_t               endp;
  size_t                  regionsize;
  NaClErrorCode           rcode;

  memp = nap->mem_start + NACL_TRAMPOLINE_END;
  endp = nap->mem_start + nap->static_text_end;
  regionsize = endp - memp;
  if (endp < memp) {
    return LOAD_NO_MEMORY;
  }

  if (nap->skip_validator) {
    NaClLog(LOG_ERROR, "VALIDATION SKIPPED.\n");
    return LOAD_OK;
  } else {
    rcode = NaClValidateCode(nap, NACL_TRAMPOLINE_END,
                             (uint8_t *) memp, regionsize);
    if (LOAD_OK != rcode) {
      if (nap->ignore_validator_result) {
        NaClLog(LOG_ERROR, "VALIDATION FAILED: continuing anyway...\n");
        rcode = LOAD_OK;
      } else {
        NaClLog(LOG_ERROR, "VALIDATION FAILED.\n");
        NaClLog(LOG_ERROR,
                "Run sel_ldr in debug mode to ignore validation failure.\n");
        NaClLog(LOG_ERROR,
                "Run ncval <module-name> for validation error details.\n");
        rcode = LOAD_VALIDATION_FAILED;
      }
    }
  }
  return rcode;
}
