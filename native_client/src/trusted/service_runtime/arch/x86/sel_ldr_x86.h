/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#ifndef SERVICE_RUNTIME_ARCH_X86_SEL_LDR_H__
#define SERVICE_RUNTIME_ARCH_X86_SEL_LDR_H__ 1

/* to make LDT_ENTRIES available */
#if NACL_WINDOWS
# define LDT_ENTRIES 8192
#elif NACL_OSX
# define LDT_ENTRIES 8192
#elif NACL_LINUX
# include <asm/ldt.h>
#endif

#include "native_client/src/trusted/service_runtime/arch/x86/nacl_ldt_x86.h"


#if NACL_BUILD_SUBARCH == 32
# define NACL_MAX_ADDR_BITS  (30)
# define NACL_THREAD_MAX     LDT_ENTRIES  /* cannot be larger */
#elif NACL_BUILD_SUBARCH == 64
# define NACL_MAX_ADDR_BITS  (32)
# define NACL_THREAD_MAX     LDT_ENTRIES  /* can be larger */
#else
# error "Did Intel or AMD introduce the 128-bit x86?"
#endif

#define NACL_NOOP_OPCODE    0x90
#define NACL_HALT_OPCODE    0xf4
#define NACL_HALT_LEN       1           /* length of halt instruction */
#define NACL_HALT_WORD      0xf4f4f4f4U

#endif /* SERVICE_RUNTIME_ARCH_X86_SEL_LDR_H__ */
