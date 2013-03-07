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

#include <signal.h>
#include <glib.h>
#include "src/platform/portability.h"
#include "src/platform/nacl_signal.h"
#include "src/main/manifest_setup.h"
#include "src/main/nacl_globals.h"
#include "src/main/zlog.h"

#define MAX_NACL_HANDLERS 16

struct NaClSignalNode {
  struct NaClSignalNode *next;
  NaClSignalHandler func;
  int id;
};

static struct NaClSignalNode *s_FirstHandler = NULL;
static struct NaClSignalNode *s_FreeList = NULL;
static struct NaClSignalNode s_SignalNodes[MAX_NACL_HANDLERS];
static int handle_signals = 1;

ssize_t NaClSignalErrorMessage(const char *msg)
{
  /*
   * We cannot use NaClLog() in the context of a signal handler: it is
   * too complex.  However, write() is signal-safe.
   */
  size_t len_t = strlen(msg);
  int len = (int) len_t;

  /*
   * Write uses int not size_t, so we may wrap the length and/or
   * generate a negative value.  Only print if it matches.
   */
  if ((len > 0) && (len_t == (size_t) len))
    return (ssize_t) write(STDOUT_FILENO, msg, len);

  return 0;
}

/*
 * Return non-zero if the signal context is currently executing in an
 * untrusted environment.
 */
int NaClSignalContextIsUntrusted(const struct NaClSignalContext *sigCtx)
{
  if(sigCtx->prog_ctr >= START_OF_USER_SPACE
      && sigCtx->prog_ctr < END_OF_USER_SPACE) return 1;
  return 0;
}

/* d'b(REPORT): updated */
enum NaClSignalResult NaClSignalHandleAll(int signum, void *ctx)
{
  struct NaClSignalContext sigCtx;
  static char msg[SIGNAL_STRLEN]; /* stack cannot be used */

  /*
   * Return an 8 bit error code which is -signal to
   * simulate normal OS behavior
   */
  NaClSignalContextFromHandler(&sigCtx, ctx);

  /* set zvm state */
  g_snprintf(msg, SIGNAL_STRLEN, "Signal %d from %strusted code: Halting at 0x%012lX",
      signum, NaClSignalContextIsUntrusted(&sigCtx) ? "un" : "", sigCtx.prog_ctr);
  SetExitState(msg);

  NaClExit(EINTR);
  return NACL_SIGNAL_RETURN; /* unreachable */
}

int NaClSignalHandlerAdd(NaClSignalHandler func) {
  int id = 0;

  ZLOGFAIL(func == NULL, EFAULT, FAILED_MSG);

  /* If we have room... */
  if (s_FreeList) {
    /* Update the free list. */
    struct NaClSignalNode *add = s_FreeList;
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

enum NaClSignalResult NaClSignalHandlerFind(int signal, void *ctx) {
  enum NaClSignalResult result = NACL_SIGNAL_SEARCH;
  struct NaClSignalNode *pNode;

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

void SetSignalHandling(int a)
{
  handle_signals = a;
}

void NaClSignalHandlerInit()
{
  int a;

  /* return if signals handling is not enabled */
  if(!handle_signals) return;

  /* Build the free list */
  for(a = 0; a < MAX_NACL_HANDLERS; a++)
  {
    s_SignalNodes[a].next = s_FreeList;
    s_SignalNodes[a].id = a + 1;
    s_FreeList = &s_SignalNodes[a];
  }

  NaClSignalHandlerInitPlatform();

  /* In stand-alone mode (sel_ldr) we handle all signals. */
  NaClSignalHandlerAdd(NaClSignalHandleAll);
}

void NaClSignalHandlerFini()
{
  /* return if signals handling is not enabled */
  if(!handle_signals) return;

  /* We try to lock, but since we are shutting down, we ignore failures. */
  NaClSignalHandlerFiniPlatform();
}
