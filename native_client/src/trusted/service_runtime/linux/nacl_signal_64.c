/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include <signal.h>
#include <sys/ucontext.h>

#include "native_client/src/trusted/service_runtime/nacl_signal.h"

/*
 * Definition of the POSIX ucontext_t for Linux can be found in:
 * /usr/include/sys/ucontext.h
 */

/*
 * Fill a signal context structure from the raw platform dependent
 * signal information.
 */
void NaClSignalContextFromHandler(struct NaClSignalContext *sigCtx,
                                  const void *rawCtx) {
  const ucontext_t *uctx = (const ucontext_t *) rawCtx;
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
  sigCtx->r8  = mctx->gregs[REG_R8];
  sigCtx->r9  = mctx->gregs[REG_R9];
  sigCtx->r10 = mctx->gregs[REG_R10];
  sigCtx->r11 = mctx->gregs[REG_R11];
  sigCtx->r12 = mctx->gregs[REG_R12];
  sigCtx->r13 = mctx->gregs[REG_R13];
  sigCtx->r14 = mctx->gregs[REG_R14];
  sigCtx->r15 = mctx->gregs[REG_R15];
  sigCtx->flags = mctx->gregs[REG_EFL];

  /* Linux stores CS, GS, FS, PAD into one 64b word. */
  sigCtx->cs = (uint32_t) (mctx->gregs[REG_CSGSFS] & 0xFFFF);
  sigCtx->gs = (uint32_t) ((mctx->gregs[REG_CSGSFS] >> 16) & 0xFFFF);
  sigCtx->fs = (uint32_t) ((mctx->gregs[REG_CSGSFS] >> 32) & 0xFFFF);

  /*
   * TODO(noelallen) Pull from current context, since they must be
   * the same.
   */
  sigCtx->ds = 0;
  sigCtx->ss = 0;
}


/*
 * Update the raw platform dependent signal information from the
 * signal context structure.
 */
void NaClSignalContextToHandler(void *rawCtx,
                                const struct NaClSignalContext *sigCtx) {
  ucontext_t *uctx = (ucontext_t *) rawCtx;
  mcontext_t *mctx = &uctx->uc_mcontext;

  mctx->gregs[REG_RIP] = sigCtx->prog_ctr;
  mctx->gregs[REG_RSP] = sigCtx->stack_ptr;

  mctx->gregs[REG_RAX] = sigCtx->rax;
  mctx->gregs[REG_RBX] = sigCtx->rbx;
  mctx->gregs[REG_RCX] = sigCtx->rcx;
  mctx->gregs[REG_RDX] = sigCtx->rdx;
  mctx->gregs[REG_RSI] = sigCtx->rsi;
  mctx->gregs[REG_RDI] = sigCtx->rdi;
  mctx->gregs[REG_RBP] = sigCtx->rbp;
  mctx->gregs[REG_R8]  = sigCtx->r8;
  mctx->gregs[REG_R9]  = sigCtx->r9;
  mctx->gregs[REG_R10] = sigCtx->r10;
  mctx->gregs[REG_R11] = sigCtx->r11;
  mctx->gregs[REG_R12] = sigCtx->r12;
  mctx->gregs[REG_R13] = sigCtx->r13;
  mctx->gregs[REG_R14] = sigCtx->r14;
  mctx->gregs[REG_R15] = sigCtx->r15;
  mctx->gregs[REG_EFL] = sigCtx->flags;

  /* Linux stores CS, GS, FS, PAD into one 64b word. */
  mctx->gregs[REG_CSGSFS] = ((uint64_t) (sigCtx->cs & 0xFFFF))
                          | (((uint64_t) (sigCtx->gs & 0xFFFF)) << 16)
                          | (((uint64_t) (sigCtx->fs & 0xFFFF)) << 32);

  /*
   * We do not support modification of DS & SS in 64b, so
   * we do not push them back into the context.
   */
}



