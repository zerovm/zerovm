/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_ADDRESS_SETS_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_ADDRESS_SETS_H__

/*
 * address_sets.h - Implements a bit set of addresses that is used by branch
 * validation to check if branches are safe.
 */

#include "native_client/src/trusted/validator/types_memory_model.h"

struct NaClValidatorState;

/* Models a set of addresses using an an array of possible addresses,
 * where the last 3 bits are unioned together using a bit mask. This cuts
 * down on the memory footprint of the address table.
 */
typedef uint8_t* NaClAddressSet;

/* Return true if the corresponding address is in the given address set. */
uint8_t NaClAddressSetContains(NaClAddressSet set,
                               NaClPcAddress address,
                               struct NaClValidatorState* state);

/* Adds the given address to the given address set. */
void NaClAddressSetAdd(NaClAddressSet set, NaClPcAddress address,
                       struct NaClValidatorState* state);

/* Create an address set for the range 0..Size. */
NaClAddressSet NaClAddressSetCreate(NaClMemorySize size);

/* frees the memory of the address set back to the system. */
void NaClAddressSetDestroy(NaClAddressSet set);

/* Returns the array size to use for the given memory size. */
size_t NaClAddressSetArraySize(NaClMemorySize size);

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_ADDRESS_SETS_H__ */
