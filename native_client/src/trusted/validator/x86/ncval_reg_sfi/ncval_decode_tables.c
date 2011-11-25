/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncval_decode_tables.h"

#include "native_client/src/trusted/validator/x86/decoder/ncopcode_desc.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_decode_tables.h"

#if NACL_TARGET_SUBARCH == 64
# include "native_client/src/trusted/validator/x86/decoder/gen/ncval_reg_sfi_opcode_table_64.h"
#else
# include "native_client/src/trusted/validator/x86/decoder/gen/ncval_reg_sfi_opcode_table_32.h"
#endif

static const NaClDecodeTables kDecoderTables = {
  g_Operands,
  g_Opcodes + 0,
  g_LookupTable + 0,
  g_PrefixOpcode + 0,
  g_Opcodes + 0,
  kNaClPrefixTable,
  g_OpcodeSeq
};

const struct NaClDecodeTables* kNaClValDecoderTables = &kDecoderTables;
