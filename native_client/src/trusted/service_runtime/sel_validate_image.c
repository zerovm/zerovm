/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/validator/ncvalidate.h"

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

int NaClValidateCode(struct NaClApp *nap, uintptr_t guest_addr,
                     uint8_t *data, size_t size) {
  NaClValidationStatus status = NaClValidationSucceeded;
  enum NaClSBKind sb_kind = NACL_SB_DEFAULT;
  if (nap->validator_stub_out_mode) {
    /* In stub out mode, we do two passes.  The second pass acts as a
       sanity check that bad instructions were indeed overwritten with
       allowable HLTs. */
    status = NACL_SUBARCH_NAME(ApplyValidator,
                               NACL_TARGET_ARCH,
                               NACL_TARGET_SUBARCH)(
                                   sb_kind,
                                   NaClApplyValidationDoStubout,
                                   guest_addr, data, size,
                                   nap->bundle_size, TRUE);
  }
  if (status == NaClValidationSucceeded) {
    status = NACL_SUBARCH_NAME(ApplyValidator,
                               NACL_TARGET_ARCH,
                               NACL_TARGET_SUBARCH)(
                                   sb_kind,
                                   NaClApplyCodeValidation,
                                   guest_addr, data, size,
                                   nap->bundle_size, TRUE);
  }
  return NaClValidateStatus(status);
}

int NaClValidateCodeReplacement(struct NaClApp *nap, uintptr_t guest_addr,
                                uint8_t *data_old, uint8_t *data_new,
                                size_t size) {
  enum NaClSBKind sb_kind = NACL_SB_DEFAULT;
  if (nap->validator_stub_out_mode) return LOAD_BAD_FILE;

  if ((guest_addr % nap->bundle_size) != 0 ||
      (size % nap->bundle_size) != 0) {
    return LOAD_BAD_FILE;
  }

  return NaClValidateStatus(
      NACL_SUBARCH_NAME(ApplyValidatorCodeReplacement,
                        NACL_TARGET_ARCH,
                        NACL_TARGET_SUBARCH)
      (sb_kind, guest_addr, data_old, data_new, size, nap->bundle_size));
}

int NaClCopyCode(struct NaClApp *nap, uintptr_t guest_addr,
                 uint8_t *data_old, uint8_t *data_new,
                 size_t size) {
  enum NaClSBKind sb_kind = NACL_SB_DEFAULT;
  return NaClValidateStatus(
      NACL_SUBARCH_NAME(ApplyValidatorCopy,
                        NACL_TARGET_ARCH,
                        NACL_TARGET_SUBARCH)
      (sb_kind, guest_addr, data_old, data_new, size, nap->bundle_size));
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
