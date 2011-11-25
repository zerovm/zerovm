/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Defines register layouts for x86 32 bit hardware registers for
 * the use in modrm.
 */


#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NACL_REGS32_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NACL_REGS32_H_

#include "native_client/src/trusted/validator/x86/decoder/ncopcode_desc.h"

/* Define the number of general purpose registers defined for the given
 * subarchitecture.
 */
#define NACL_REG_TABLE_SIZE_32 8

/* Defines that the nacl register is not defined. */
#define NACL_REGISTER_UNDEFINED_32 (-1)

/* Define the available 8-bit registers, for the given subarchitecture,
 * assuming the REX prefix is not present.
 * Note: The order is important, and is based on the indexing values used
 * in the ModRm and SIB bytes (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTable8NoRex_32[NACL_REG_TABLE_SIZE_32] = {
  RegAL,
  RegCL,
  RegDL,
  RegBL,
  RegAH,
  RegCH,
  RegDH,
  RegBH,
};

/* Define the available 8-bit registers, for the given subarchitecture,
 * assuming the rex prefix is present.
 * Note: The order is important, and is based on the indexing values used
 * in the ModRm and SIB bytes (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTable8Rex_32[NACL_REG_TABLE_SIZE_32] = {
  RegAL,
  RegCL,
  RegDL,
  RegBL,
  RegAH,
  RegCH,
  RegDH,
  RegBH
};

/* Define the available 16-bit registers, for the given subarchitecture.
 * Note: The order is important, and is based on the indexing values used
 * in the ModRm and SIB bytes (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTable16_32[NACL_REG_TABLE_SIZE_32] = {
  RegAX,
  RegCX,
  RegDX,
  RegBX,
  RegSP,
  RegBP,
  RegSI,
  RegDI,
};

/* Define the available 32-bit registers, for the given subarchitecture.
 * Note: The order is important, and is based on the indexing values used
 * in the ModRm and SIB bytes (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTable32_32[NACL_REG_TABLE_SIZE_32] = {
  RegEAX,
  RegECX,
  RegEDX,
  RegEBX,
  RegESP,
  RegEBP,
  RegESI,
  RegEDI,
};

/* Define the available 64-bit registers, for the given subarchitecture.
 * Note: The order is important, and is based on the indexing values used
 * in the ModRm and SIB bytes (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTable64_32[NACL_REG_TABLE_SIZE_32] = {
  RegUnknown,
  RegUnknown,
  RegUnknown,
  RegUnknown,
  RegUnknown,
  RegUnknown,
  RegUnknown,
  RegUnknown
};

/* Define the available Mmx registers, for the given subarchitecture.
 * Note: The order is important, and is based on the indexing values
 * used in the ModRm and SIB bytes (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTableMmx_32[NACL_REG_TABLE_SIZE_32] = {
  RegMMX0,
  RegMMX1,
  RegMMX2,
  RegMMX3,
  RegMMX4,
  RegMMX5,
  RegMMX6,
  RegMMX7,
};

/* Define the available Xmm registers, for the given subarchitecture.
 * Note: The order is important, and is based on the indexing values
 * used in the ModRm and SIB bytes (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTableXmm_32[NACL_REG_TABLE_SIZE_32] = {
  RegXMM0,
  RegXMM1,
  RegXMM2,
  RegXMM3,
  RegXMM4,
  RegXMM5,
  RegXMM6,
  RegXMM7,
};

/* Defines the available control registers, for the given subarchitecture.
 * Note: The order is important, and is based on the indexing values
 * used in the ModRm byte (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTableC_32[NACL_REG_TABLE_SIZE_32] = {
  RegCR0,
  RegCR1,
  RegCR2,
  RegCR3,
  RegCR4,
  RegCR5,
  RegCR6,
  RegCR7,
};

/* Defines the available debug registers, for the given subarchitecture.
 * Note: The order is important, and is based on the indexing values
 * used in the ModRm byte (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTableD_32[NACL_REG_TABLE_SIZE_32] = {
  RegDR0,
  RegDR1,
  RegDR2,
  RegDR3,
  RegDR4,
  RegDR5,
  RegDR6,
  RegDR7,
};

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NACL_REGS_H_ */
