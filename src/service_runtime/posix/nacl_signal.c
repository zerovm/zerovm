/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include "src/service_runtime/nacl_signal.h"
#include "src/service_runtime/sel_ldr.h"

/*
 * This module is based on the Posix signal model.  See:
 * http://www.opengroup.org/onlinepubs/009695399/functions/sigaction.html
 */

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

/*
 * TODO(noelallen) split these macros and conditional compiles
 * into architecture specific files. Bug #955
 */

/* Use 4K more than the minimum to allow breakpad to run. */
#define SIGNAL_STACK_SIZE (SIGSTKSZ + 4096)
#define STACK_GUARD_SIZE NACL_PAGESIZE

#ifdef SIGSTKFLT
#define SIGNAL_COUNT 10
static int s_Signals[SIGNAL_COUNT] = {
  SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGBUS, SIGFPE, SIGSEGV, SIGSTKFLT, SIGXFSZ, SIGXCPU
};
#else
#define SIGNAL_COUNT 9
static int s_Signals[SIGNAL_COUNT] = {
  SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGBUS, SIGFPE, SIGSEGV, SIGXFSZ, SIGXCPU
};
#endif

static struct sigaction s_OldActions[SIGNAL_COUNT];

int NaClSignalStackAllocate(void **result) {
  /*
   * We use mmap() to allocate the signal stack for two reasons:
   *
   * 1) By page-aligning the memory allocation (which malloc() does
   * not do for small allocations), we avoid allocating any real
   * memory in the common case in which the signal handler is never
   * run.
   *
   * 2) We get to create a guard page, to guard against the unlikely
   * occurrence of the signal handler both overrunning and doing so in
   * an exploitable way.
   */
  uint8_t *stack = mmap(NULL, SIGNAL_STACK_SIZE + STACK_GUARD_SIZE,
                        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
                        -1, 0);
  if (stack == MAP_FAILED) {
    return 0;
  }
  /* We assume that the stack grows downwards. */
  if (mprotect(stack, STACK_GUARD_SIZE, PROT_NONE) != 0) {
    NaClLog(LOG_FATAL, "Failed to mprotect() the stack guard page:\n\t%s\n",
      strerror(errno));
  }
  *result = stack;
  return 1;
}

static void FindAndRunHandler(int sig, siginfo_t *info, void *uc) {
  if (NaClSignalHandlerFind(sig, uc) == NACL_SIGNAL_SEARCH) {
    int a;

    /* If we need to keep searching, try the old signal handler. */
    for (a = 0; a < SIGNAL_COUNT; a++) {
      /* If we handle this signal */
      if (s_Signals[a] == sig) {
        /* If this is a real sigaction pointer... */
        if (s_OldActions[a].sa_flags & SA_SIGINFO) {
          /* then call the old handler. */
          s_OldActions[a].sa_sigaction(sig, info, uc);
          break;
        }
        /* otherwise check if it is a real signal pointer */
        if ((s_OldActions[a].sa_handler != SIG_DFL) &&
            (s_OldActions[a].sa_handler != SIG_IGN)) {
          /* and call the old signal. */
          s_OldActions[a].sa_handler(sig);
          break;
        }
        /*
         * We matched the signal, but didn't handle it, so we emulate
         * the default behavior which is to exit the app with the signal
         * number as the error code.
         */
        NaClSignalErrorMessage("Failed to handle signal.\n");
        NaClExit(-sig);
      }
    }
  }
}

static void SignalCatch(int sig, siginfo_t *info, void *uc) {
  FindAndRunHandler(sig, info, uc);
}

void NaClSignalHandlerInitPlatform() {
  struct sigaction sa;
  int a;

  memset(&sa, 0, sizeof(sa));
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = SignalCatch;
  sa.sa_flags = SA_ONSTACK | SA_SIGINFO;

  /* Mask all exceptions we catch to prevent re-entry */
  for (a = 0; a < SIGNAL_COUNT; a++) {
    sigaddset(&sa.sa_mask, s_Signals[a]);
  }

  /* Install all handlers */
  for (a = 0; a < SIGNAL_COUNT; a++) {
    if (sigaction(s_Signals[a], &sa, &s_OldActions[a]) != 0) {
      NaClLog(LOG_FATAL, "Failed to install handler for %d.\n\tERR:%s\n",
                          s_Signals[a], strerror(errno));
    }
  }
}

void NaClSignalHandlerFiniPlatform() {
  int a;

  /* Remove all handlers */
  for (a = 0; a < SIGNAL_COUNT; a++) {
    if (sigaction(s_Signals[a], &s_OldActions[a], NULL) != 0) {
      NaClLog(LOG_FATAL, "Failed to unregister handler for %d.\n\tERR:%s\n",
                          s_Signals[a], strerror(errno));
    }
  }
}

/*
 * Check that signal handlers are not registered.  We want to
 * discourage Chrome or libraries from registering signal handlers
 * themselves, because those signal handlers are often not safe when
 * triggered from untrusted code.  For background, see:
 * http://code.google.com/p/nativeclient/issues/detail?id=1607
 */
void NaClSignalAssertNoHandlers() {
  int index;
  for (index = 0; index < SIGNAL_COUNT; index++) {
    int signum = s_Signals[index];
    struct sigaction sa;
    if (sigaction(signum, NULL, &sa) != 0) {
      NaClLog(LOG_FATAL, "NaClSignalAssertNoHandlers: "
              "sigaction() call failed\n");
    }
    if ((sa.sa_flags & SA_SIGINFO) != 0
        ? sa.sa_sigaction != NULL
        : (sa.sa_handler != SIG_DFL && sa.sa_handler != SIG_IGN)) {
      NaClLog(LOG_FATAL, "NaClSignalAssertNoHandlers: "
              "A signal handler is registered for signal %d.  "
              "Did Breakpad register this?\n", signum);
    }
  }
}
