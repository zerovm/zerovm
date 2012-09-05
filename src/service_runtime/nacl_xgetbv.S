/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This exists only because Windows 64 doesn't support trivial inline assembly.
 * Visual Studio 2010 SP1 has an intrinsic _xgetbv() for this.
 * But earlier versions do not.
 */

#include "src/service_runtime/nacl_config.h"

        .text

.globl IDENTIFIER(NaClXGETBV)
        HIDDEN(NaClXGETBV)
IDENTIFIER(NaClXGETBV):

        /*
         * This is "xgetbv" (no operands).  Some assemblers don't know it yet.
         */
        .byte 0x0f, 0x01, 0xd0
        shlq $32, %rdx
        orq %rdx, %rax
        ret
