/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * x86_insts.h - Holds inline routines for commonly used (simple)
 * functions in x86_insts.h. Used to speed up code. Inlined routines
 * correspond to the following functions in x86_insts.h, but with an
 * 'Inline' suffix:
 *
 *   modrm_mod
 *   modrm_rm
 *   modrm_reg
 *   modrm_opcode
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_X86_INSTS_INL_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_X86_INSTS_INL_H__

#include "native_client/src/trusted/validator/x86/x86_insts.h"

/* Accessors for the ModRm byte. */
static INLINE uint8_t modrm_modInline(uint8_t modrm) {
  return ((modrm >> 6) & 0x03);
}

static INLINE uint8_t modrm_rmInline(uint8_t modrm) {
  return (modrm & 0x07);
}

static INLINE uint8_t modrm_regInline(uint8_t modrm) {
  return ((modrm >> 3) & 0x07);
}

static INLINE uint8_t modrm_opcodeInline(uint8_t modrm) {
  return modrm_regInline(modrm);
}

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_X86_INSTS_INL_H__ */
