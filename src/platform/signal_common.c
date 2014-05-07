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

#include <stdio.h>
#include <errno.h>
#include "src/platform/signal.h"
#include "src/main/setup.h"
#include "src/main/config.h"
#include "src/main/zlog.h"

#define MAX_HANDLERS 16

struct SignalNode {
  struct SignalNode *next;
  SignalHandler func;
  int id;
};

/*
 * Architecture specific context object.  Register order matches that
 * found in src/src/gdb_rsp/abi.cc, which allows us to use an
 * abi context (GDB ordered context), and a signal context interchangably.
 * In addition, we use common names for the stack and program counter to
 * allow functions which use them to avoid conditional compilation.
 * TODO(d'b): it almost the duplicate of struct ThreadContext. there can be only one.
 */
struct SignalContext {
  uint64_t rax;
  uint64_t rbx;
  uint64_t rcx;
  uint64_t rdx;
  uint64_t rsi;
  uint64_t rdi;
  uint64_t rbp;
  uint64_t stack_ptr;
  uint64_t r8;
  uint64_t r9;
  uint64_t r10;
  uint64_t r11;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;
  uint64_t prog_ctr;
  uint32_t flags;
  uint32_t cs;
  uint32_t ss;
  uint32_t ds;
  uint32_t es;
  uint32_t fs;
  uint32_t gs;
};

static struct SignalNode *s_FirstHandler = NULL;
static struct SignalNode *s_FreeList = NULL;
static struct SignalNode s_SignalNodes[MAX_HANDLERS];

/*
 * Return non-zero if the signal context is currently executing in an
 * untrusted environment.
 */
static int SignalContextIsUntrusted(const struct SignalContext *sigCtx)
{
  if(sigCtx->prog_ctr >= UNTRUSTED_START
      && sigCtx->prog_ctr < UNTRUSTED_END) return 1;
  return 0;
}

/*
 * Fill a signal context structure from the raw platform dependent
 * signal information.
 */
static void SignalContextFromHandler(
    struct SignalContext *sigCtx, const void *rawCtx)
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

/* exit zerovm with "interrupted syscall" code and the message */
static enum SignalResult SignalHandleAll(int signum, void *ctx)
{
  struct SignalContext sigCtx;
  char msg[SIGNAL_STRLEN];

  /* TODO(d'b): is it a proper solution? */
  static int busy = 0;
  if(busy) return NACL_SIGNAL_RETURN;
  busy = 1;

  /*
   * Return an 8 bit error code which is -signal to
   * simulate normal OS behavior
   */
  SignalContextFromHandler(&sigCtx, ctx);

  /* set zerovm state */
  if(signum == SIGALRM)
    g_snprintf(msg, SIGNAL_STRLEN, "session timeout");
  else if(signum == SIGXFSZ)
    g_snprintf(msg, SIGNAL_STRLEN, "disk quota exceeded");
  else
    g_snprintf(msg, SIGNAL_STRLEN,
        "Signal %d from %strusted code: Halting at 0x%012lX", signum,
        SignalContextIsUntrusted(&sigCtx) ? "un" : "", sigCtx.prog_ctr);

  SessionDtor(EINTR, msg);
  return NACL_SIGNAL_RETURN; /* unreachable */
}

/*
 * Add a signal handler to the front of the list.
 * Returns an id for the handler or returns 0 on failure.
 * This function is not thread-safe and should only be
 * called at startup.
 */
static int SignalHandlerAdd(SignalHandler func)
{
  int id = 0;

  ZLOGFAIL(func == NULL, EFAULT, FAILED_MSG);

  /* If we have room... */
  if(s_FreeList)
  {
    /* Update the free list. */
    struct SignalNode *add = s_FreeList;
    s_FreeList = add->next;

    /* Construct the node. */
    add->func = func;
    add->next = s_FirstHandler;

    /* Add node to the head. */
    s_FirstHandler = add;
    id = add->id;
  }

  return id;
}

enum SignalResult SignalHandlerFind(int signal, void *ctx)
{
  enum SignalResult result = NACL_SIGNAL_SEARCH;
  struct SignalNode *pNode;

  /* Iterate through handlers */
  pNode = s_FirstHandler;
  while (pNode) {
    result = pNode->func(signal, ctx);

    /* If we are not asking for the search to continue... */
    if (NACL_SIGNAL_SEARCH != result) break;

    pNode = pNode->next;
  }

  return result;
}

void SignalHandlerInit()
{
  int a;

  /* Build the free list */
  for(a = 0; a < MAX_HANDLERS; a++)
  {
    s_SignalNodes[a].next = s_FreeList;
    s_SignalNodes[a].id = a + 1;
    s_FreeList = &s_SignalNodes[a];
  }

  SignalHandlerInitPlatform();

  /* In stand-alone mode we handle all signals */
  SignalHandlerAdd(SignalHandleAll);
}

/* We try to lock, but since we are shutting down, we ignore failures */
void SignalHandlerFini()
{
  SignalHandlerFiniPlatform();
}
