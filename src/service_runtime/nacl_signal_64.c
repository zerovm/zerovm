/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */
#include <sys/ucontext.h>

#include "src/service_runtime/nacl_signal.h"
#include "src/service_runtime/sel_rt_64.h"

/*
 * Definition of the POSIX ucontext_t for Linux can be found in:
 * /usr/src/include/sys/ucontext.h
 */

/*
 * Fill a signal context structure from the raw platform dependent
 * signal information.
 */
void NaClSignalContextFromHandler(struct NaClSignalContext *sigCtx, const void *rawCtx)
{
  const ucontext_t *uctx = (const ucontext_t *)rawCtx;
  const mcontext_t *mctx = &uctx->uc_mcontext;

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

  /*
   * TODO(noelallen) Pull from current context, since they must be
   * the same.
   */
  sigCtx->ds = 0;
  sigCtx->ss = 0;
}

#ifdef DISABLE_RDTSC
/* store the signal context into the thread context */
void NaClThreadContextFromHandler(struct NaClThreadContext *thread_ctx, const void *rawCtx)
{
  const ucontext_t *uctx = (const ucontext_t *)rawCtx;
  const mcontext_t *mctx = &uctx->uc_mcontext;

  thread_ctx->new_prog_ctr = mctx->gregs[REG_RIP];
  thread_ctx->prog_ctr = mctx->gregs[REG_RIP]; /* do we need set this? */
  thread_ctx->sysret = 0; /* meaningless since there was no syscall. just clear it */

  thread_ctx->rax = mctx->gregs[REG_RAX];
  thread_ctx->rbx = mctx->gregs[REG_RBX];
  thread_ctx->rcx = mctx->gregs[REG_RCX];
  thread_ctx->rdx = mctx->gregs[REG_RDX];
  thread_ctx->rsi = mctx->gregs[REG_RSI];
  thread_ctx->rdi = mctx->gregs[REG_RDI];
  thread_ctx->rbp = mctx->gregs[REG_RBP];
  thread_ctx->rsp = mctx->gregs[REG_RSP];

  thread_ctx->r8 = mctx->gregs[REG_R8];
  thread_ctx->r9 = mctx->gregs[REG_R9];
  thread_ctx->r10 = mctx->gregs[REG_R10];
  thread_ctx->r11 = mctx->gregs[REG_R11];
  thread_ctx->r12 = mctx->gregs[REG_R12];
  thread_ctx->r13 = mctx->gregs[REG_R13];
  thread_ctx->r14 = mctx->gregs[REG_R14];
  thread_ctx->r15 = mctx->gregs[REG_R15];
}
#endif
