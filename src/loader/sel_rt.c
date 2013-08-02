/*
 * Copyright 2009 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
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

#include <errno.h>
#include "src/loader/sel_ldr.h"

nacl_reg_t GetStackPtr(void)
{
  nacl_reg_t rsp;

  asm("mov %%rsp, %0" : "=r" (rsp) : );
  return rsp;
}

uintptr_t GetThreadCtxSp(struct ThreadContext *th_ctx)
{
  return (uintptr_t)th_ctx->rsp;
}

void SetThreadCtxSp(struct ThreadContext *th_ctx, uintptr_t sp)
{
  th_ctx->rsp = (nacl_reg_t)sp;
}

int ThreadContextCtor(struct ThreadContext *ntcp,
                      struct NaClApp       *nap,
                      nacl_reg_t           prog_ctr,
                      nacl_reg_t           stack_ptr,
                      uint32_t             tls_idx)
{
  ntcp->rax = 0;
  ntcp->rbx = 0;
  ntcp->rcx = 0;
  ntcp->rdx = 0;

  ntcp->rbp = stack_ptr; /* must be a valid stack addr! */
  ntcp->rsi = 0;
  ntcp->rdi = 0;
  ntcp->rsp = stack_ptr;

  ntcp->r8 = 0;
  ntcp->r9 = 0;
  ntcp->r10 = 0;
  ntcp->r11 = 0;

  ntcp->r12 = 0;
  ntcp->r13 = 0;
  ntcp->r14 = 0;
  ntcp->r15 = nap->mem_start;

  ntcp->prog_ctr = NaClUserToSys(nap, prog_ctr);
  ntcp->new_prog_ctr = 0;
  ntcp->sysret = -EINVAL;

  ntcp->tls_base = NULL;
  ntcp->tls_idx = tls_idx;

  return 1;
}
