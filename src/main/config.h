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
 *
 * NOTE: This header is ALSO included by assembler files and hence
 *       must not include any C code
 */
#ifndef CONFIG_H_
#define CONFIG_H_

/* maximum number of elf program headers allowed. */
#define NACL_MAX_PROGRAM_HEADERS 128

/*
 * macros to provide uniform access to identifiers from assembly due
 * to different C -> asm name mangling conventions and other platform-specific
 * requirements
 */
#define IDENTIFIER(n)  n
#define HIDDEN(n)  .hidden IDENTIFIER(n)

/*
 * this value must be consistent with NaCl compiler flags
 * -falign-functions -falign-labels -and nacl-align.
 */
#define NACL_BLOCK_SHIFT 5
#define NACL_INSTR_BLOCK_SIZE    (1 << NACL_BLOCK_SHIFT)

/* this must be a multiple of the system page size */
#define NACL_PAGESHIFT           12
#define NACL_PAGESIZE            (1U << NACL_PAGESHIFT)

#define NACL_MAP_PAGESHIFT       16
#define NACL_MAP_PAGESIZE        (1U << NACL_MAP_PAGESHIFT)

#if NACL_MAP_PAGESHIFT < NACL_PAGESHIFT
# error "NACL_MAP_PAGESHIFT smaller than NACL_PAGESHIFT"
#endif

/*
 * Macro for the start address of the trampolines
 * The first 64KB (16 pages) are inaccessible.  On x86, this is to prevent
 * addr16/data16 attacks.
 */
#define NACL_SYSCALL_START_ADDR  (16 << NACL_PAGESHIFT)

/*
 * Syscall trampoline code have a block size that may differ from the
 * alignment restrictions of the executable.  The ELF executable's
 * alignment restriction (16 or 32) defines what are potential entry
 * points, so the trampoline region must still respect that.  We
 * prefill the trampoline region with HLT, so non-syscall entry points
 * will not cause problems as long as our trampoline instruction
 * sequence never grows beyond 16 bytes, so the "odd" potential entry
 * points for a 16-byte aligned ELF will not be able to jump into the
 * middle of the trampoline code.
 */
#define NACL_SYSCALL_BLOCK_SHIFT 5
#define NACL_SYSCALL_BLOCK_SIZE  (1 << NACL_SYSCALL_BLOCK_SHIFT)

/*
 * the extra space for the trampoline syscall code and the thread
 * contexts must be a multiple of the page size.
 *
 * The address space begins with a 64KB region that is inaccessible to
 * handle NULL pointers and also to reinforce protection agasint abuse of
 * addr16/data16 prefixes.
 * NACL_TRAMPOLINE_START gives the address of the first trampoline.
 * NACL_TRAMPOLINE_END gives the address of the first byte after the
 * trampolines.
 */
#define NACL_NULL_REGION_SHIFT   16
#define NACL_TRAMPOLINE_START    (1 << NACL_NULL_REGION_SHIFT)
#define NACL_TRAMPOLINE_SHIFT    16
#define NACL_TRAMPOLINE_SIZE     (1 << NACL_TRAMPOLINE_SHIFT)
#define NACL_TRAMPOLINE_END      (NACL_TRAMPOLINE_START + NACL_TRAMPOLINE_SIZE)

/*
 * Extra required space at the end of static text (and dynamic text,
 * if any). The intent is to stop a thread from "walking off the end"
 * of a text region into another.  Four bytes suffices for all
 * currently supported architectures (halt is one byte on x86-32 and
 * x86-64, and 4 bytes on ARM), but for x86 we want to provide
 * paranoia-in-depth: if the NX bit (x86-64, arm) or the %cs segment
 * protection (x86-32) fails on weird borderline cases and the
 * validator also fails to detect a weird instruction sequence, we may
 * have the following situation: a partial multi-byte instruction is
 * placed on the end of the last page, and by "absorbing" the trailing
 * HALT instructions (esp if there are none) as immediate data (or
 * something similar), cause the PC to go outside of the untrusted
 * code space, possibly into data memory.  By requiring 32 bytes of
 * space to fill with HALT instructions, we (attempt to) ensure that
 * such op-code absorption cannot happen, and at least one of these
 * HALTs will cause the untrusted thread to abort, and take down the
 * whole NaCl app.
 */
#define NACL_HALT_SLED_SIZE      32

#define NACL_USERRET_FIX         (0x8)
#define NACL_SYSARGS_FIX         (-0x18)
#define NACL_SYSCALLRET_FIX      (0x10)
/*
 * System V Application Binary Interface, AMD64 Architecture Processor
 * Supplement, at http://www.x86-64.org/documentation/abi.pdf, section
 * 3.2.2 discusses stack alignment.
 * d'b: since zerovm gave up the command line and environment support
 * the user stack has now constant content 56 bytes long
 */
#define STACK_USER_DATA_SIZE     56

/* d'b: macro definitions for the user space allocation */
#define FOURGIG     (((size_t) 1) << 32)
#define GUARDSIZE   (10 * FOURGIG)
#define R15_CONST   ((void*)0x440000000000)
#define RELATIVE_MMAP (MAP_ANONYMOUS | MAP_NORESERVE | MAP_PRIVATE)
#define ABSOLUTE_MMAP (RELATIVE_MMAP | MAP_FIXED)
#define START_OF_USER_SPACE ((uintptr_t)R15_CONST)
#define END_OF_USER_SPACE (START_OF_USER_SPACE + FOURGIG + 2 * GUARDSIZE)
#define LEAST_USER_HEAP_SIZE (8*1024*1024)
#define STACK_SIZE 0x1000000
#define MEM_START ((uintptr_t)R15_CONST + GUARDSIZE)

#endif  /* CONFIG_H_ */
