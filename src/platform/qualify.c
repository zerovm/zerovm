/*
 * Copyright 2010 The Native Client Authors.  All rights reserved.
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

/*
 * POSIX-specific routines for verifying that Data Execution Prevention is
 * functional.
 */

#include <setjmp.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <glib.h>
#include "src/main/zlog.h"
#include "src/platform/qualify.h"
#include "src/platform/nacl_macros.h"

/*
 * A void nullary function.  We generate functions of this type in the heap and
 * stack to prove that we cannot call them.  See GenerateThunk, below.
 */
typedef void (*nacl_void_thunk)();

/* Assembled equivalent of "ret" */
#define INST_RET 0xC3
#define EXPECTED_SIGNAL SIGSEGV

static struct sigaction previous_sigaction;
static struct sigaction try_sigaction;
static sigjmp_buf try_state;

static void signal_catch(int sig)
{
  siglongjmp(try_state, sig);
}

static void setup_signals()
{
  try_sigaction.sa_handler = signal_catch;
  sigemptyset(&try_sigaction.sa_mask);
  try_sigaction.sa_flags = SA_RESETHAND;

  (void) sigaction(EXPECTED_SIGNAL, &try_sigaction, &previous_sigaction);
}

static void restore_signals()
{
  (void) sigaction(EXPECTED_SIGNAL, &previous_sigaction, 0);
}

/*
 * Generates an architecture-specific void thunk sequence into the provided
 * buffer.  Returns a pointer to the thunk (which may be offset into the
 * buffer, depending on alignment constraints etc.).
 *
 * If the buffer is too small for the architecture's thunk sequence, returns
 * NULL.  In general the buffer should be more than 4 bytes.
 *
 * NOTE: the implementation of this function is architecture-specific, and can
 * be found beneath arch/ in this directory.
 */
static nacl_void_thunk GenerateThunk(char *buf, size_t size_in_bytes)
{
  /* Place a "ret" at buf */
  if (size_in_bytes < 1) return 0;

  *buf = (char) INST_RET;

  /*
   * ISO C prevents a direct data->function cast, because the pointers aren't
   * guaranteed to be the same size.  For our platforms this is fine, but we
   * verify at compile time anyway before tricking the compiler:
   */
  NACL_ASSERT_SAME_SIZE(char *, nacl_void_thunk);
  return (nacl_void_thunk) (uintptr_t) buf;
}

/* Returns 1 if Data Execution Prevention is present and working */
static int AttemptToExecuteData()
{
  int result;
  char *thunk_buffer = g_malloc(64);
  nacl_void_thunk thunk;

  /* d'b: small fixes */
  if(thunk_buffer == NULL) return 0;
  thunk = GenerateThunk(thunk_buffer, 64);
  setup_signals();

  if(0 == sigsetjmp(try_state, 1))
  {
    thunk();
    result = 0;
  }
  else
  {
    result = 1;
  }

  restore_signals();
  g_free(thunk_buffer);
  return result;
}

void RunSelQualificationTests()
{
  ZLOGFAIL(!AttemptToExecuteData(), EFAULT, "dep not supported");
}
