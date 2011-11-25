/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Defines register layouts for x86 32/64 bit hardware registers for
 * the use in modrm.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NACL_REGS_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NACL_REGS_H_

#if NACL_TARGET_SUBARCH == 64

#include  "native_client/src/trusted/validator/x86/nacl_regs64.h"

#define NACL_REG_TABLE_SIZE NACL_REG_TABLE_SIZE_64
#define NACL_REGISTER_UNDEFINED NACL_REGISTER_UNDEFINED_64
#define NaClRegTable8NoRex NaClRegTable8NoRex_64
#define NaClRegTable8Rex NaClRegTable8Rex_64
#define NaClRegTable16 NaClRegTable16_64
#define NaClRegTable32 NaClRegTable32_64
#define NaClRegTable64 NaClRegTable64_64
#define NaClRegTableMmx NaClRegTableMmx_64
#define NaClRegTableXmm NaClRegTableXmm_64
#define NaClRegTableC NaClRegTableC_64
#define NaClRegTableD NaClRegTableD_64

#else

#include  "native_client/src/trusted/validator/x86/nacl_regs32.h"

#define NACL_REG_TABLE_SIZE NACL_REG_TABLE_SIZE_32
#define NACL_REGISTER_UNDEFINED NACL_REGISTER_UNDEFINED_32
#define NaClRegTable8NoRex NaClRegTable8NoRex_32
#define NaClRegTable8Rex NaClRegTable8Rex_32
#define NaClRegTable16 NaClRegTable16_32
#define NaClRegTable32 NaClRegTable32_32
#define NaClRegTable64 NaClRegTable64_32
#define NaClRegTableMmx NaClRegTableMmx_32
#define NaClRegTableXmm NaClRegTableXmm_32
#define NaClRegTableC NaClRegTableC_32
#define NaClRegTableD NaClRegTableD_32

#endif

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NACL_REGS_H_ */
