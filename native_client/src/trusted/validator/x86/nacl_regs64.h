/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Defines register layouts for x86 64 bit hardware registers for
 * the use in modrm.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NACL_REGS64_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NACL_REGS64_H_

#include "native_client/src/trusted/validator/x86/decoder/ncopcode_desc.h"

/* Define the number of general purpose registers defined for the given
 * subarchitecture.
 */
#define NACL_REG_TABLE_SIZE_64 16

/* Defines that the nacl register is not defined. */
#define NACL_REGISTER_UNDEFINED_64 (-1)

/* Define the available 8-bit registers, for the given subarchitecture,
 * assuming the REX prefix is not present.
 * Note: The order is important, and is based on the indexing values used
 * in the ModRm and SIB bytes (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTable8NoRex_64[NACL_REG_TABLE_SIZE_64] = {
  RegAL,
  RegCL,
  RegDL,
  RegBL,
  RegAH,
  RegCH,
  RegDH,
  RegBH,
  RegUnknown,
  RegUnknown,
  RegUnknown,
  RegUnknown,
  RegUnknown,
  RegUnknown,
  RegUnknown,
  RegUnknown
};

/* Define the available 8-bit registers, for the given subarchitecture,
 * assuming the rex prefix is present.
 * Note: The order is important, and is based on the indexing values used
 * in the ModRm and SIB bytes (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTable8Rex_64[NACL_REG_TABLE_SIZE_64] = {
  RegAL,
  RegCL,
  RegDL,
  RegBL,
  RegSPL,
  RegBPL,
  RegSIL,
  RegDIL,
  RegR8B,
  RegR9B,
  RegR10B,
  RegR11B,
  RegR12B,
  RegR13B,
  RegR14B,
  RegR15B
};

/* Define the available 16-bit registers, for the given subarchitecture.
 * Note: The order is important, and is based on the indexing values used
 * in the ModRm and SIB bytes (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTable16_64[NACL_REG_TABLE_SIZE_64] = {
  RegAX,
  RegCX,
  RegDX,
  RegBX,
  RegSP,
  RegBP,
  RegSI,
  RegDI,
  RegR8W,
  RegR9W,
  RegR10W,
  RegR11W,
  RegR12W,
  RegR13W,
  RegR14W,
  RegR15W,
};

/* Define the available 32-bit registers, for the given subarchitecture.
 * Note: The order is important, and is based on the indexing values used
 * in the ModRm and SIB bytes (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTable32_64[NACL_REG_TABLE_SIZE_64] = {
  RegEAX,
  RegECX,
  RegEDX,
  RegEBX,
  RegESP,
  RegEBP,
  RegESI,
  RegEDI,
  RegR8D,
  RegR9D,
  RegR10D,
  RegR11D,
  RegR12D,
  RegR13D,
  RegR14D,
  RegR15D
};

/* Define the available 64-bit registers, for the given subarchitecture.
 * Note: The order is important, and is based on the indexing values used
 * in the ModRm and SIB bytes (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTable64_64[NACL_REG_TABLE_SIZE_64] = {
  RegRAX,
  RegRCX,
  RegRDX,
  RegRBX,
  RegRSP,
  RegRBP,
  RegRSI,
  RegRDI,
  RegR8,
  RegR9,
  RegR10,
  RegR11,
  RegR12,
  RegR13,
  RegR14,
  RegR15
};

/* Define the available Mmx registers, for the given subarchitecture.
 * Note: The order is important, and is based on the indexing values
 * used in the ModRm and SIB bytes (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTableMmx_64[NACL_REG_TABLE_SIZE_64] = {
  RegMMX0,
  RegMMX1,
  RegMMX2,
  RegMMX3,
  RegMMX4,
  RegMMX5,
  RegMMX6,
  RegMMX7,
  /* Intentionally repeat values, since Rex.B/R has no effect. */
  RegMMX0,
  RegMMX1,
  RegMMX2,
  RegMMX3,
  RegMMX4,
  RegMMX5,
  RegMMX6,
  RegMMX7
};

/* Define the available Xmm registers, for the given subarchitecture.
 * Note: The order is important, and is based on the indexing values
 * used in the ModRm and SIB bytes (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTableXmm_64[NACL_REG_TABLE_SIZE_64] = {
  RegXMM0,
  RegXMM1,
  RegXMM2,
  RegXMM3,
  RegXMM4,
  RegXMM5,
  RegXMM6,
  RegXMM7,
  RegXMM8,
  RegXMM9,
  RegXMM10,
  RegXMM11,
  RegXMM12,
  RegXMM13,
  RegXMM14,
  RegXMM15
};

/* Defines the available control registers, for the given subarchitecture.
 * Note: The order is important, and is based on the indexing values
 * used in the ModRm byte (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTableC_64[NACL_REG_TABLE_SIZE_64] = {
  RegCR0,
  RegCR1,
  RegCR2,
  RegCR3,
  RegCR4,
  RegCR5,
  RegCR6,
  RegCR7,
  RegCR8,
  RegCR9,
  RegCR10,
  RegCR11,
  RegCR12,
  RegCR13,
  RegCR14,
  RegCR15,
};

/* Defines the available debug registers, for the given subarchitecture.
 * Note: The order is important, and is based on the indexing values
 * used in the ModRm byte (and the REX prefix if appropriate).
 */
static const NaClOpKind NaClRegTableD_64[NACL_REG_TABLE_SIZE_64] = {
  RegDR0,
  RegDR1,
  RegDR2,
  RegDR3,
  RegDR4,
  RegDR5,
  RegDR6,
  RegDR7,
  RegDR8,
  RegDR9,
  RegDR10,
  RegDR11,
  RegDR12,
  RegDR13,
  RegDR14,
  RegDR15,
};

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NACL_REGS64_H_ */
