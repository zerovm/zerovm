/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SERVICE_RUNTIME_ARCH_ARM_SEL_LDR_H__
#define SERVICE_RUNTIME_ARCH_ARM_SEL_LDR_H__ 1

#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

/* NOTE: we hope to unify this among archtectures */
#define NACL_MAX_ADDR_BITS      30

#define NACL_THREAD_MAX         8192  /* arbitrary, can be larger */

#if defined(NACL_TARGET_ARM_THUMB2_MODE)
#define NACL_NOOP_OPCODE        0x46c0      /* mov r8, r8 */
#define NACL_HALT_OPCODE        0xbe00      /* bkpt 0x0000 */
#define NACL_HALT_LEN           2           /* length of halt instruction */
#else
#define NACL_NOOP_OPCODE        0xe1a00000  /* mov r0, r0 */
#define NACL_HALT_OPCODE        0xe1266676  /* bkpt 6666 */
#define NACL_HALT_LEN           4           /* length of halt instruction */
#endif  /* defined(NACL_TARGET_ARM_THUMB2_MODE) */
#define NACL_HALT_WORD          NACL_HALT_OPCODE

struct NaClApp;
struct NaClThreadContext;

uint32_t NaClGetThreadCombinedDescriptor(struct NaClThreadContext *user);

void NaClSetThreadCombinedDescriptor(struct NaClThreadContext *user,
                                     uint32_t tls_idx);

#endif /* SERVICE_RUNTIME_ARCH_ARM_SEL_LDR_H__ */
