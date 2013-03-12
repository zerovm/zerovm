/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
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

#include <sys/ucontext.h>

#include "src/platform/nacl_signal.h"
#include "src/loader/sel_rt.h"

void NaClSignalContextFromHandler(struct NaClSignalContext *sigCtx, const void *rawCtx)
{
  const ucontext_t *uctx = (const ucontext_t *)rawCtx;
  const mcontext_t *mctx = &uctx->uc_mcontext;

#ifdef __USE_GNU
  sigCtx->prog_ctr = mctx->gregs[REG_RIP];
  sigCtx->stack_ptr = mctx->gregs[REG_RSP];

  sigCtx->rax = mctx->gregs[REG_RAX];
  sigCtx->rbx = mctx->gregs[REG_RBX];
  sigCtx->rcx = mctx->gregs[REG_RCX];
  sigCtx->rdx = mctx->gregs[REG_RDX];
  sigCtx->rsi = mctx->gregs[REG_RSI];
  sigCtx->rdi = mctx->gregs[REG_RDI];
  sigCtx->rbp = mctx->gregs[REG_RBP];
  sigCtx->r8 = mctx->gregs[REG_R8];
  sigCtx->r9 = mctx->gregs[REG_R9];
  sigCtx->r10 = mctx->gregs[REG_R10];
  sigCtx->r11 = mctx->gregs[REG_R11];
  sigCtx->r12 = mctx->gregs[REG_R12];
  sigCtx->r13 = mctx->gregs[REG_R13];
  sigCtx->r14 = mctx->gregs[REG_R14];
  sigCtx->r15 = mctx->gregs[REG_R15];
  sigCtx->flags = mctx->gregs[REG_EFL];

  /* Linux stores CS, GS, FS, PAD into one 64b word. */
  sigCtx->cs = (uint32_t)(mctx->gregs[REG_CSGSFS] & 0xFFFF);
  sigCtx->gs = (uint32_t)((mctx->gregs[REG_CSGSFS] >> 16) & 0xFFFF);
  sigCtx->fs = (uint32_t)((mctx->gregs[REG_CSGSFS] >> 32) & 0xFFFF);
#endif

  /* TODO(noelallen) Pull from current context, since they must be the same */
  sigCtx->ds = 0;
  sigCtx->ss = 0;
}
