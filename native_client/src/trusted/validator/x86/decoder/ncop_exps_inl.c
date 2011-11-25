/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
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
#include "native_client/src/trusted/validator/x86/decoder/ncop_exps.h"

static INLINE NaClOpKind NaClGetExpRegisterInline(NaClExp* node) {
  assert(node->kind == ExprRegister);
  return (NaClOpKind) node->value;
}

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NCOP_EXPS_INL_H_ */
