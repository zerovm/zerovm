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
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */

/*
 * NaCl code uses the standard x86-64 calling convention, whereas the
 * service runtime might be using the x86-64 calling convention or the
 * Microsoft x86-64 calling convention.  See sel_rt_64.h
 * for a discussion and pointers to references.
 *
 * Available registers: rax, rcx, rdx, r8, r9, r10, r11, since these
 * are all caller saved.  r15 is nominally callee saved, but for NaCl
 * code r15 is rzp, the reserved zero-address base pointer for the
 * start of the NaCl module's address space.  Since its value is
 * easily regenerated, we can trash it here.
 *
 * Arguments: rdi, rsi, rdx, rcx, r8, r9 (standard x86-64; rest on stack)
 *            rcx, rdx, r8, r9 (Microsoft; rest on stack)
 *
 * This means that we will need to conditionalize the trampoline code
 * based on which compiler was used for the service_runtime.
 */

#include "src/main/config.h"

        .text
        .globl IDENTIFIER(NaCl_trampoline_code)
HIDDEN(NaCl_trampoline_code)
IDENTIFIER(NaCl_trampoline_code):
        mov $0x657a696c6167656c, %rax
        .globl IDENTIFIER(NaCl_trampoline_call_target)
HIDDEN(NaCl_trampoline_call_target)
IDENTIFIER(NaCl_trampoline_call_target):
        call *%rax
        /* call IDENTIFIER(SyscallSeg) */
        hlt
        .globl IDENTIFIER(NaCl_trampoline_code_end)
HIDDEN(NaCl_trampoline_code_end)
IDENTIFIER(NaCl_trampoline_code_end):

        /*
         * DispatchThunk to DispatchThunkEnd below is a code template
         * for dynamically patched code.  The trampoline code will call to
         * it, and the dispatch thunk will jump to SyscallSeg which is the
         * actual syscall handler.  The rationale for this double trampoline
         * is that the trampoline code above is readable by the untrusted
         * NaCl module, and by doing this we hide the address of SyscallSeg
         * from the untrusted code.  The dispatch thunk can be relatively
         * easily randomized, and if the trusted code were built with
         * Address Space Layout Randomization enabled, would not leak ASLR
         * information to the untrusted code.
         *
         * See sel_ldr_x86_64.c and {linux,win}/sel_memory.c for
         * how the thunk is generated.
         */
        .text
        .globl IDENTIFIER(NaClDispatchThunk)
HIDDEN(NaClDispatchThunk)
IDENTIFIER(NaClDispatchThunk):
        mov $0x736962616e6e6163, %rax
        .globl IDENTIFIER(NaClDispatchThunk_jmp_target)
HIDDEN(NaClDispatchThunk_jmp_target)
IDENTIFIER(NaClDispatchThunk_jmp_target):
        jmp *%rax
        /* jmp IDENTIFIER(SyscallSeg) */
        .globl IDENTIFIER(NaClDispatchThunkEnd)
HIDDEN(NaClDispatchThunkEnd)
IDENTIFIER(NaClDispatchThunkEnd):
