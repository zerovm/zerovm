/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Implement the ApplyValidator API for the x86-64 architecture. */
#include <assert.h>
#include "src/platform/nacl_log.h"
#include "src/validator/ncvalidate.h"
#include "src/validator/validation_cache.h"
#include "src/validator/x86/ncval_reg_sfi/ncvalidate_iter.h"

/* Be sure the correct compile flags are defined for this. */
#if NACL_ARCH(NACL_TARGET_ARCH) != NACL_x86
# error("Can't compile, target is for x86-64")
#else
# if NACL_TARGET_SUBARCH != 64
#  error("Can't compile, target is for x86-64")
# endif
#endif

NaClValidationStatus NaClValidatorSetup_x86_64(
    intptr_t guest_addr,
    size_t size,
    int readonly_text,
    const NaClCPUFeaturesX86 *cpu_features,
    struct NaClValidatorState** vstate_ptr) {
  *vstate_ptr = NaClValidatorStateCreate(guest_addr, size, RegR15,
                                         readonly_text, cpu_features);
  return (*vstate_ptr == NULL)
      ? NaClValidationFailedOutOfMemory
      : NaClValidationSucceeded;     /* or at least to this point! */
}

NaClValidationStatus NACL_SUBARCH_NAME(ApplyValidator, x86, 64) (
    uintptr_t guest_addr,
    uint8_t *data,
    size_t size,
    int stubout_mode,
    int readonly_text,
    const NaClCPUFeaturesX86 *cpu_features,
    struct NaClValidationCache *cache) {
  struct NaClValidatorState *vstate;
  NaClValidationStatus status;
  void *query = NULL;

  /* Check that the given parameter values are supported. */
  if (stubout_mode && readonly_text)
    return NaClValidationFailedNotImplemented;

  if (!NaClArchSupported(cpu_features))
    return NaClValidationFailedCpuNotSupported;

  /* Don't cache in stubout mode. */
  if (stubout_mode)
    cache = NULL;

  /* If the validation caching interface is available, perform a query. */
  if (cache != NULL)
    query = cache->CreateQuery(cache->handle);
  if (query != NULL) {
    const char validator_id[] = "x86-64";
    cache->AddData(query, (uint8_t *) validator_id, sizeof(validator_id));
    cache->AddData(query, (uint8_t *) cpu_features, sizeof(*cpu_features));
    cache->AddData(query, data, size);
    if (cache->QueryKnownToValidate(query)) {
      cache->DestroyQuery(query);
      return NaClValidationSucceeded;
    }
  }

  /* Init then validator state. */
  status = NaClValidatorSetup_x86_64(
      guest_addr, size, readonly_text, cpu_features, &vstate);
  if (status != NaClValidationSucceeded) {
    if (query != NULL)
      cache->DestroyQuery(query);
    return status;
  }
  NaClValidatorStateSetLogVerbosity(vstate, LOG_ERROR);
  NaClValidatorStateSetDoStubOut(vstate, stubout_mode);

  /* Validate. */
  NaClValidateSegment(data, guest_addr, size, vstate);
  status = (NaClValidatesOk(vstate) || stubout_mode) ?
      NaClValidationSucceeded : NaClValidationFailed;

  /* Cache the result if validation succeded and the code was not modified. */
  if (query != NULL) {
    /* Don't cache the result if the code is modified. */
    if (status == NaClValidationSucceeded && !NaClValidatorDidStubOut(vstate))
      cache->SetKnownToValidate(query);
    cache->DestroyQuery(query);
  }

  NaClValidatorStateDestroy(vstate);
  return status;
}

NaClValidationStatus NACL_SUBARCH_NAME(ApplyValidatorCodeReplacement, x86, 64)
    (uintptr_t guest_addr,
     uint8_t *data_old,
     uint8_t *data_new,
     size_t size,
     const NaClCPUFeaturesX86 *cpu_features) {
  NaClValidationStatus status;
  struct NaClValidatorState *vstate;

  /* Check that the given parameter values are supported. */
  if (!NaClArchSupported(cpu_features))
    return NaClValidationFailedCpuNotSupported;

  /* Init then validator state. */
  status = NaClValidatorSetup_x86_64(guest_addr, size, FALSE,
                                     cpu_features, &vstate);
  if (status != NaClValidationSucceeded)
    return status;
  NaClValidatorStateSetLogVerbosity(vstate, LOG_ERROR);

  /* Validate. */
  NaClValidateSegmentPair(data_old, data_new, guest_addr, size, vstate);
  status = NaClValidatesOk(vstate) ?
      NaClValidationSucceeded : NaClValidationFailed;

  NaClValidatorStateDestroy(vstate);
  return status;
}
