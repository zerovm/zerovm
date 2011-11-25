/*
 * Copyright 2010 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * POSIX-specific routines for verifying that Data Execution Prevention is
 * functional.
 */

#include <setjmp.h>
#include <stdlib.h>
#include <signal.h>

#include "native_client/src/trusted/platform_qualify/nacl_dep_qualify.h"
#include "native_client/src/trusted/platform_qualify/linux/sysv_shm_and_mmap.h"

#if NACL_OSX && NACL_BUILD_ARCH == NACL_x86 && NACL_BUILD_SUBARCH == 64
# define EXPECTED_SIGNAL SIGBUS
#else
# define EXPECTED_SIGNAL SIGSEGV
#endif

static struct sigaction previous_sigaction;
static struct sigaction try_sigaction;
static sigjmp_buf try_state;

static void signal_catch(int sig) {
  siglongjmp(try_state, sig);
}

static void setup_signals() {
  try_sigaction.sa_handler = signal_catch;
  sigemptyset(&try_sigaction.sa_mask);
  try_sigaction.sa_flags = SA_RESETHAND;

  (void) sigaction(EXPECTED_SIGNAL, &try_sigaction, &previous_sigaction);
}

static void restore_signals() {
  (void) sigaction(EXPECTED_SIGNAL, &previous_sigaction, 0);
}

/*
 * Returns 1 if Data Execution Prevention is present and working.
 */
int NaClAttemptToExecuteData() {
  int result;
  char *thunk_buffer = malloc(64);
  nacl_void_thunk thunk = NaClGenerateThunk(thunk_buffer, 64);

  setup_signals();

  if (0 == sigsetjmp(try_state, 1)) {
    thunk();
    result = 0;
  } else {
    result = 1;
  }

  restore_signals();
  return result;
}
