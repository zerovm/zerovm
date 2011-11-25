/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This code gets executed when switching from a 64-bit nacl module to
 * the 64-bit service. NaClSyscallSeg is the lcall target from the
 * syscall trampoline code, and this code is responsible for figuring
 * out the identity of the thread, saving the user registers, finish
 * restoring the segment registers (and getting out of the sandbox),
 * and actually invoking the C system call handler code.
 */

#include "native_client/src/trusted/service_runtime/nacl_config.h"

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

        /*
         * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
         *
         * Open coded TLS (or TSD) access, for all of our target host
         * OS/toolchains.  If the compiler / runtime conventions for
         * how to access TLS or TSD changes, this code will break
         * mysteriously.
         *
         * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
         */
#if NACL_LINUX
        /*
         * Linux uses TLS.
         * The spelling is slightly different when we need to be PIC-friendly.
         */
# ifdef __PIC__
        movq    nacl_thread_index@GOTTPOFF(%rip), %rax
        movl    %fs:(%rax), %eax
# else
        movl    %fs:nacl_thread_index@TPOFF, %eax
# endif
#elif NACL_OSX
        /*
         * This assumes a particular implementation of OS X's
         * pthread_getspecific(), which we check for in NaClTlsInit().
         */
        movl    _nacl_thread_index_tls_offset(%rip), %eax
        movl    %gs:(%rax), %eax
#elif NACL_WINDOWS
        /*
         * NOTE: This code should match the code in platform/win/test_tls.S!
         * If you change this code, please update that file as well.
         *
         * The following two lines generate this instruction:
         *   ba XX XX XX XX    movl $nacl_thread_index@SECREL, %edx
         *
         * SECREL is a relocation type which gives the offset of
         * nacl_thread_index within the TLS template section.  GNU
         * binutils' assembler does not have a syntax for generating
         * this relocation as part of an instruction.  It only has the
         * .secrel32 syntax for generating the relocated value as a
         * 32-bit literal.  We use this here for generating the
         * instruction by hand.
         *
         * For background, see:
         * http://code.google.com/p/nativeclient/issues/detail?id=2155
         */
        .byte 0xba
        .secrel32 nacl_thread_index

        mov _tls_index(%rip), %ecx
        movq %gs:0x58, %rax
        /* Get the address of this module (executable/DLL)'s TLS area. */
        movq (%rax,%rcx,8), %rax
        /* Get the value of nacl_thread_index from the TLS area. */
        movl (%rdx,%rax), %eax
#else
# error "What OS/compiler is the service runtime being compiled with?"
#endif
        /* check NaClThreadContext in sel_rt_64.h for the offsets */
        leaq    IDENTIFIER(nacl_user)(%rip), %rdx
        leaq    (%rdx,%rax,8), %rdx
        movq    0x0(%rdx), %rdx

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
        leaq    0x0(%rdx,%rax,8), %rdx
        movq    0x0(%rdx), %rdx

        movq    0x38(%rdx), %rsp
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
#if NACL_LINUX || NACL_OSX
        movq    %rax, %rdi
#elif NACL_WINDOWS
        movq    %rax, %rcx
#else
# error "What OS/compiler is the service runtime being compiled with?"
#endif
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

#if NACL_WINDOWS && defined(_WIN64)
/* On Windows64 this has to be done via out-of-line assembly code */
        HIDDEN(NaClGetStackPtr)
IDENTIFIER(NaClGetStackPtr):
        movq %rsp, %rax
        ret
#endif
