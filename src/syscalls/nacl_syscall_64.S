/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*
 * Copyright (c) 2012, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This code gets executed when switching from a 64-bit nacl module to
 * the 64-bit service. NaClSyscallSeg is the lcall target from the
 * syscall trampoline code, and this code is responsible for figuring
 * out the identity of the thread, saving the user registers, finish
 * restoring the segment registers (and getting out of the sandbox),
 * and actually invoking the C system call handler code.
 */

#include "src/main/nacl_config.h"

/*
 * On untrusted stack:
 *      return-addr-to-caller-of-trampoline
 *      return-addr-to-trampoline (essentially syscall number)
 *
 * This code must save the syscall arguments so that they can be
 * accessed in an uniformed way, regardless of whether the service
 * runtime was compiled using gcc (NACL_LINUX and NACL_OSX) or
 * using MS Studio (NACL_WINDOWS).
 */
        .text
        .globl  IDENTIFIER(NaClGetStackPtr)
        .globl  IDENTIFIER(NaClSyscallSeg)
        HIDDEN(NaClSyscallSeg)
IDENTIFIER(NaClSyscallSeg):
        cld

        /*
         * pushl   %r9d
         * pushl   %r8d
         * pushl   %ecx
         * pushl   %edx
         * pushl   %esi
         * pushl   %edi
         */
        subq $0x18, %rsp
        movl %r9d, 0x14(%rsp)
        movl %r8d, 0x10(%rsp)
        movl %ecx, 0x0c(%rsp)
        movl %edx, 0x08(%rsp)
        movl %esi, 0x04(%rsp)
        movl %edi, 0x00(%rsp)

        /* rax, rdi, rsi, rdx, rcx, r8, r9 are usable for scratch */

        /* check NaClThreadContext in sel_rt_64.h for the offsets */
        leaq    IDENTIFIER(nacl_user)(%rip), %rdx
        movq    (%rdx), %rdx

        /* only save the callee saved registers */
        movq    %rbx, 0x8(%rdx)
        movq    %rbp, 0x20(%rdx)
        movq    %rsp, %rbx
        addq    $0x18, %rbx      /* tramp ret */
        movq    %rbx, 0x38(%rdx) /* get syscall args via the saved %rsp+0x30 */
        movq    %r12, 0x60(%rdx)
        movq    %r13, 0x68(%rdx)
        movq    %r14, 0x70(%rdx)
        /* r15 need not be saved, since it is immutable from user code */

        /* restore system registers needed to call into C code */
        leaq    IDENTIFIER(nacl_sys)(%rip), %rdx
        movq    (%rdx), %rdx

        movq    0x38(%rdx), %rsp

        /*
         * d'b: added to align stack to 16 bytes, in the future
         * for other platform the alignment should be set to appropriate
         * values.
         * note: w/o alignment some avx instructions aren't working
         */
        andq	$-0xf, %rsp
        /*
         * hic sunt dracones
         *
         * According to the AMD64 ABI, %rsp must be 0 mod 16 here.
         * (0 mod 32, if __m256 is on stack, which won't be the case.)
         *
         * Note for NACL_WINDOWS, %rsp must be pre-subtracted by $0x20
         * for shadow space for Windows x86-64 compiler's non-standard
         * calling convention's varargs save area.
         *
         * These properties are not enforced here; rather, the code in
         * nacl_switch_to_app_64.c handles this.
         */
        movq    %rax, %rdi
        call    IDENTIFIER(NaClSyscallCSegHook)
        hlt

        /*
         * untrusted stack after call to NaClSyscallCSegHook:
         *
         * 0x20   0x8 return-addr-to-caller-of-trampoline
         * 0x18   0x0 return-addr-to-trampoline (essentially syscall number)
         * 0x14 -0x04 r9
         * 0x10 -0x08 r8
         * 0x0c -0x0c rcx
         * 0x08 -0x10 rdx
         * 0x04 -0x14 rsi
         * 0x00 -0x18 rdi
         *
         * trusted stack after call to NaClSyscallCSegHook:
         *
         *      return addr (to the hlt)
         */

        /* noret */
