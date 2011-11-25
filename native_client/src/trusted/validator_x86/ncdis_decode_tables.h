/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * API for set of decoder tables to be used by ncdis.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCDIS_DECODE_TABLES_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCDIS_DECODE_TABLES_H__

#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

struct NaClDecodeTables;

/* Defines decoding tables used by ncdis to decode instructions using
 * the "full" instruction model.
 */
extern const struct NaClDecodeTables* kNaClDecoderTables;

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCDIS_DECODE_TABLES_H__ */
