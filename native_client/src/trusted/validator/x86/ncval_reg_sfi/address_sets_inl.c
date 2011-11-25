/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * address_sets_inl.c - Holds inline routines for commonly used (simple)
 * functions in address_sets.h. Used to speed up code. Inlined routines
 * correspond to the following functions in address_sets.h, but with
 * an 'Inline' suffix:
 *
 *   NaClAddressSetAdd
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_ADDRESS_SETS_INL_C__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_ADDRESS_SETS_INL_C__

#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/address_sets.h"

/* Model the offset created by removing the bottom three bits of a PcAddress. */
typedef NaClPcAddress NaClPcOffset;

/* Model the set of possible 3-bit tails of possible PcAddresses. */
static const uint8_t nacl_pc_address_masks[8] = {
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

/* Convert the 3 lower bits of an address into the corresponding address mask
 * to use.
 */
static INLINE uint8_t NaClPcAddressToMask(NaClPcAddress address) {
  return nacl_pc_address_masks[(int) (address & (NaClPcAddress)0x7)];
}

/* Convert an address into the corresponding offset in an address table.
 * That is, strip off the last three bits, since these remaining bits
 * will be encoded using the union of address masks in the address table.
 */
static INLINE NaClPcOffset NaClPcAddressToOffset(NaClPcAddress address) {
  return address >> 3;
}

/* Returns true if the given address is within the code segment. Generates
 * error messages if it isn't.
 */
static INLINE Bool NaClCheckAddressRange(NaClPcAddress address,
                                         NaClValidatorState* state) {
  if (address < state->vbase) {
    NaClValidatorPcAddressMessage(LOG_ERROR, state, address,
                                  "Jump to address before code block.\n");
    return FALSE;
  }
  if (address >= state->vlimit) {
    NaClValidatorPcAddressMessage(LOG_ERROR, state, address,
                                  "Jump to address beyond code block limit.\n");
    return FALSE;
  }
  return TRUE;
}

static INLINE void NaClAddressSetAddInline(NaClAddressSet set,
                                           NaClPcAddress address,
                                           NaClValidatorState* state) {
  if (NaClCheckAddressRange(address, state)) {
    NaClPcAddress offset = address - state->vbase;
    DEBUG(NaClLog(LOG_INFO,
                  "Address set add: %"NACL_PRIxNaClPcAddress"\n", address));
    set[NaClPcAddressToOffset(offset)] |= NaClPcAddressToMask(offset);
  }
}

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_REG_SFI_ADDRESS_SETS_INL_C__ */
