/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/include/portability_io.h"
#include "src/service_runtime/nacl_signal.h"
#include "src/manifest/manifest_setup.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/zlog.h"

#define MAX_NACL_HANDLERS 16

struct NaClSignalNode {
  struct NaClSignalNode *next;
  NaClSignalHandler func;
  int id;
};

static struct NaClSignalNode *s_FirstHandler = NULL;
static struct NaClSignalNode *s_FreeList = NULL;
static struct NaClSignalNode s_SignalNodes[MAX_NACL_HANDLERS];

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
  /* d'b(REPORT) */
  return !gnap->trusted_code;
}

/* d'b(REPORT): updated */
enum NaClSignalResult NaClSignalHandleAll(int signal, void *ctx) {
  struct NaClSignalContext sigCtx;

  /*
   * Return an 8 bit error code which is -signal to
   * simulate normal OS behavior
   */
  NaClSignalContextFromHandler(&sigCtx, ctx);

  /* set zvm state */
  SNPRINTF(gnap->zvm_state, SIGNAL_STRLEN,
      "Signal %d from %strusted code: Halting at 0x%lX", signal,
      NaClSignalContextIsUntrusted(&sigCtx) ? "un" : "", sigCtx.prog_ctr);

  NaClExit((-signal) & 0xFF);
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

void NaClSignalHandlerInit() {
  int a;

  /* Build the free list */
  for (a = 0; a < MAX_NACL_HANDLERS; a++) {
    s_SignalNodes[a].next = s_FreeList;
    s_SignalNodes[a].id = a + 1;
    s_FreeList = &s_SignalNodes[a];
  }

  NaClSignalHandlerInitPlatform();

  /* In stand-alone mode (sel_ldr) we handle all signals. */
  NaClSignalHandlerAdd(NaClSignalHandleAll);
}

void NaClSignalHandlerFini() {
  /* We try to lock, but since we are shutting down, we ignore failures. */
  NaClSignalHandlerFiniPlatform();
}
