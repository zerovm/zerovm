/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * ncop_exps_inl.c - Holds inline routines for commonly used (simple)
 * functions in ncop_exps.h. Used to speed up code. Inlined routines
 * correspond to the following functions in ncop_exps.h, but with an
 * 'Inline' suffix:
 *
 *    NaClGetExpRegister
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NCOP_EXPS_INL_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NCOP_EXPS_INL_H_

#include <assert.h>
#include "src/validator/x86/decoder/ncop_exps.h"

static INLINE NaClOpKind NaClGetExpRegisterInline(NaClExp* node) {
  assert(node->kind == ExprRegister);
  return (NaClOpKind) node->value;
}

/* Return the sign extended integer in the given expr node. */
static INLINE int64_t NaClGetExprSignedValue(NaClExp* node) {
  if (node->flags & NACL_EFLAG(ExprSize8)) {
    return (int8_t) node->value;
  } else if (node->flags & NACL_EFLAG(ExprSize16)) {
    return (int16_t) node->value;
  } else if (node->flags & NACL_EFLAG(ExprSize32)) {
    return (int32_t) node->value;
  } else {
    return (int64_t) node->value;
  }
}

/* Return the zero padded integer in the given expr node. */
static INLINE uint64_t NaClGetExprUnsignedValue(NaClExp* node) {
  if (node->flags & NACL_EFLAG(ExprSize8)) {
    return (uint8_t) node->value;
  } else if (node->flags & NACL_EFLAG(ExprSize16)) {
    return (uint16_t) node->value;
  } else if (node->flags & NACL_EFLAG(ExprSize32)) {
    return (uint32_t) node->value;
  } else {
    return (uint64_t) node->value;
  }
}

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NCOP_EXPS_INL_H_ */
