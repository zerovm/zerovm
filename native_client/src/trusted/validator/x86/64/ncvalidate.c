/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Implement the ApplyValidator API for the x86-64 architecture. */
#include <assert.h>
#include "native_client/src/trusted/validator/ncvalidate.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter.h"

/* Be sure the correct compile flags are defined for this. */
#if NACL_ARCH(NACL_TARGET_ARCH) != NACL_x86
# error("Can't compile, target is for x86-64")
#else
# if NACL_TARGET_SUBARCH != 64
#  error("Can't compile, target is for x86-64")
# endif
#endif

static NaClValidationStatus NaClApplyValidatorSilently_x86_64(
    uintptr_t guest_addr,
    uint8_t *data,
    size_t size,
    int bundle_size,
    Bool local_cpu) {
  CPUFeatures features;
  int is_ok;
  struct NaClValidatorState *vstate =
      NaClValidatorStateCreate(guest_addr, size, bundle_size, RegR15);
  if (vstate == NULL) return NaClValidationFailedOutOfMemory;
  NaClValidatorStateSetLogVerbosity(vstate, LOG_ERROR);
  if (!local_cpu) {
    NaClSetAllCPUFeatures(&features);
    NaClValidatorStateSetCPUFeatures(vstate, &features);
  }
  NaClValidateSegment(data, guest_addr, size, vstate);
  is_ok = NaClValidatesOk(vstate);
  NaClValidatorStateDestroy(vstate);
  return is_ok ? NaClValidationSucceeded : NaClValidationFailed;
}

NaClValidationStatus NaClApplyValidatorStubout_x86_64(
    uintptr_t guest_addr,
    uint8_t *data,
    size_t size,
    int bundle_size,
    Bool local_cpu) {
  CPUFeatures features;
  struct NaClValidatorState *vstate =
      NaClValidatorStateCreate(guest_addr, size, bundle_size, RegR15);
  if (vstate == NULL) return NaClValidationFailedOutOfMemory;
  NaClValidatorStateSetDoStubOut(vstate, TRUE);
  if (!local_cpu) {
    NaClSetAllCPUFeatures(&features);
    NaClValidatorStateSetCPUFeatures(vstate, &features);
  }
  NaClValidateSegment(data, guest_addr, size, vstate);
  NaClValidatorStateDestroy(vstate);
  return NaClValidationSucceeded;
}

NaClValidationStatus NACL_SUBARCH_NAME(ApplyValidator, x86, 64) (
    enum NaClSBKind sb_kind,
    NaClApplyValidationKind kind,
    uintptr_t guest_addr,
    uint8_t *data,
    size_t size,
    int bundle_size,
    Bool local_cpu) {
  NaClValidationStatus status = NaClValidationFailedNotImplemented;
  assert(NACL_SB_DEFAULT == sb_kind);
  if (bundle_size == 16 || bundle_size == 32) {
    if (local_cpu) {
      NaClCPUData cpu_data;
      NaClCPUDataGet(&cpu_data);
      if (!NaClArchSupported(&cpu_data))
        return NaClValidationFailedCpuNotSupported;
    }
    switch (kind) {
      case NaClApplyCodeValidation:
        status = NaClApplyValidatorSilently_x86_64(
            guest_addr, data, size, bundle_size, local_cpu);
        break;
      case NaClApplyValidationDoStubout:
        status = NaClApplyValidatorStubout_x86_64(
            guest_addr, data, size, bundle_size, local_cpu);
        break;
      default:
        break;
    }
  }
  return status;
}

static NaClValidationStatus NaClApplyValidatorPair(
    uintptr_t guest_addr,
    uint8_t *data_old,
    uint8_t *data_new,
    size_t size,
    int bundle_size) {
  int is_ok;
  struct NaClValidatorState *vstate =
      NaClValidatorStateCreate(guest_addr, size, bundle_size, RegR15);
  if (vstate == NULL) return NaClValidationFailedOutOfMemory;
  NaClValidatorStateSetLogVerbosity(vstate, LOG_ERROR);
  NaClValidateSegmentPair(data_old, data_new, guest_addr, size, vstate);
  is_ok = NaClValidatesOk(vstate);
  NaClValidatorStateDestroy(vstate);
  return is_ok ? NaClValidationSucceeded : NaClValidationFailed;
}

NaClValidationStatus NACL_SUBARCH_NAME(ApplyValidatorCodeReplacement, x86, 64)
    (enum NaClSBKind sb_kind,
     uintptr_t guest_addr,
     uint8_t *data_old,
     uint8_t *data_new,
     size_t size,
     int bundle_size) {
  NaClValidationStatus status = NaClValidationFailedNotImplemented;
  assert(NACL_SB_DEFAULT == sb_kind);
  if (bundle_size == 16 || bundle_size == 32) {
    NaClCPUData cpu_data;
    NaClCPUDataGet(&cpu_data);
    if (!NaClArchSupported(&cpu_data)) {
      status = NaClValidationFailedCpuNotSupported;
    } else {
      status = NaClApplyValidatorPair(guest_addr, data_old, data_new,
                                      size, bundle_size);
    }
  }
  return status;
}
