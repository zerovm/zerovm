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

#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <stdint.h>

#include "src/main/config.h"
#include "src/main/manifest.h"
#include "src/main/zlog.h"
#include "src/loader/userspace.h"

typedef uint64_t nacl_reg_t; /* general purpose register type */

struct ThreadContext {
  nacl_reg_t  rax,  rbx,  rcx,  rdx,  rbp,  rsi,  rdi,  rsp;
  /*          0x0,  0x8, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38 */
  nacl_reg_t  r8,     r9,  r10,  r11,  r12,  r13,  r14,  r15;
  /*          0x40, 0x48, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78 */
  nacl_reg_t  prog_ctr;  /* rip */
  /*          0x80 */
  nacl_reg_t  sysret;
  /*          0x88 */
};

/* global variables */
struct ThreadContext    *nacl_user; /* user registers storage */
struct ThreadContext    *nacl_sys;  /* zerovm registers storage */

/* TODO(d'b): try to convert inline functions to macros */
/* d'b: no checks, just does the work */
static INLINE uintptr_t NaClUserToSysAddrNullOkay(uintptr_t uaddr)
{
  return uaddr + MEM_START;
}

static INLINE uintptr_t NaClUserToSys(uintptr_t uaddr)
{
  ZLOGFAIL(0 == uaddr || ((uintptr_t) 1U << ADDR_BITS) <= uaddr, EFAULT,
      "uaddr 0x%08lx, addr space %d bits", uaddr, ADDR_BITS);
  return uaddr + MEM_START;
}

static INLINE uintptr_t NaClSysToUser(uintptr_t sysaddr)
{
  ZLOGFAIL(MEM_START + ((uintptr_t) 1U << ADDR_BITS) <= sysaddr
      || sysaddr < MEM_START, EFAULT,
      "sysaddr 0x%08lx, mem_start 0x%08lx, addr space %d bits",
      sysaddr, MEM_START, ADDR_BITS);
  return sysaddr - MEM_START;
}

static INLINE uintptr_t NaClSandboxCodeAddr(uintptr_t addr)
{
  return (((addr & ~(((uintptr_t)NACL_INSTR_BLOCK_SIZE) - 1))
           & ((((uintptr_t) 1) << 32) - 1)) + MEM_START);
}

void ThreadContextCtor(struct ThreadContext *ntcp,
                       nacl_reg_t            prog_ctr,
                       nacl_reg_t            stack_ptr);

uintptr_t GetThreadCtxSp(struct ThreadContext *th_ctx);

void SetThreadCtxSp(struct ThreadContext *th_ctx, uintptr_t sp);

nacl_reg_t GetStackPtr(void);

#endif /* CONTEXT_H_ */
