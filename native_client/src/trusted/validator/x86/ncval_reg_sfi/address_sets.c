/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.

 */

/*
 * address_sets.c - Implements a bit set of addresses that is used by branch
 * validation to check if branches are safe.
 */

#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/address_sets.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter.h"
#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter_internal.h"

/* To turn on debugging of instruction decoding, change value of
 * DEBUGGING to 1.
 */
#define DEBUGGING 0

#include "native_client/src/shared/utils/debugging.h"

#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/address_sets_inl.c"

uint8_t NaClAddressSetContains(NaClAddressSet set,
                               NaClPcAddress address,
                               NaClValidatorState* state) {
  if (NaClCheckAddressRange(address, state)) {
    NaClPcAddress offset = address - state->vbase;
    return set[NaClPcAddressToOffset(offset)] & NaClPcAddressToMask(offset);
  } else {
    return FALSE;
  }
}

void NaClAddressSetAdd(NaClAddressSet set, NaClPcAddress address,
                       NaClValidatorState* state) {
  NaClAddressSetAddInline(set, address, state);
}

size_t NaClAddressSetArraySize(NaClMemorySize size) {
  /* Be sure to add an element for partial overlaps. */
  /* TODO(karl) The cast to size_t for the number of elements may
   * cause loss of data. We need to fix this. This is a security
   * issue when doing cross-platform (32-64 bit) generation.
   */
  return (size_t) NaClPcAddressToOffset(size) + 1;
}

NaClAddressSet NaClAddressSetCreate(NaClMemorySize size) {
  return (NaClAddressSet) calloc(NaClAddressSetArraySize(size),
                                 sizeof(uint8_t));
}

void NaClAddressSetDestroy(NaClAddressSet set) {
  free(set);
}
