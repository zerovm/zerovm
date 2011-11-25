/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */

/*
 * NaCl code uses the standard x86-64 calling convention, whereas the
 * service runtime might be using the x86-64 calling convention or the
 * Microsoft x86-64 calling convention.  See arch/x86_64/sel_rt_64.h
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

#include "native_client/src/trusted/service_runtime/nacl_config.h"

        .text
        .globl IDENTIFIER(NaCl_trampoline_code)
HIDDEN(NaCl_trampoline_code)
IDENTIFIER(NaCl_trampoline_code):
        mov $0xdeadbeefcafebabe, %rax
        .globl IDENTIFIER(NaCl_trampoline_call_target)
HIDDEN(NaCl_trampoline_call_target)
IDENTIFIER(NaCl_trampoline_call_target):
        call *%rax
        /* call IDENTIFIER(NaClSyscallSeg) */
        hlt
        .globl IDENTIFIER(NaCl_trampoline_code_end)
HIDDEN(NaCl_trampoline_code_end)
IDENTIFIER(NaCl_trampoline_code_end):

        /*
         * NaClDispatchThunk to NaClDispatchThunkEnd below is a code template
         * for dynamically patched code.  The trampoline code will call to
         * it, and the dispatch thunk will jump to NaClSyscallSeg which is the
         * actual syscall handler.  The rationale for this double trampoline
         * is that the trampoline code above is readable by the untrusted
         * NaCl module, and by doing this we hide the address of NaClSyscallSeg
         * from the untrusted code.  The dispatch thunk can be relatively
         * easily randomized, and if the trusted code were built with
         * Address Space Layout Randomization enabled, would not leak ASLR
         * information to the untrusted code.
         *
         * See arch/x86_64/sel_ldr_x86_64.c and {linux,win}/sel_memory.c for
         * how the thunk is generated.
         */
        .text
        .globl IDENTIFIER(NaClDispatchThunk)
HIDDEN(NaClDispatchThunk)
IDENTIFIER(NaClDispatchThunk):
        mov $0xdeadbeefcafebabe, %rax
        .globl IDENTIFIER(NaClDispatchThunk_jmp_target)
HIDDEN(NaClDispatchThunk_jmp_target)
IDENTIFIER(NaClDispatchThunk_jmp_target):
        jmp *%rax
        /* jmp IDENTIFIER(NaClSyscallSeg) */
        .globl IDENTIFIER(NaClDispatchThunkEnd)
HIDDEN(NaClDispatchThunkEnd)
IDENTIFIER(NaClDispatchThunkEnd):
