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

#ifndef NACL_SIGNAL_H__
#define NACL_SIGNAL_H__ 1

/*
 * The nacl_signal module provides a platform independent mechanism for
 * trapping signals encountered while running a Native Client executable.
 * Signal handlers can be installed which will receive a POSIX signal number
 * and a platform dependent signal object.  Accessors are provided to convert
 * to and from architecture dependent CPU state structures.
 */

#include "src/platform/nacl_signal_64.h"
#include "src/loader/sel_rt_64.h"

EXTERN_C_BEGIN

struct NaClApp;

enum NaClSignalResult {
  NACL_SIGNAL_SEARCH,   /* Try our handler or OS */
  NACL_SIGNAL_SKIP,     /* Skip our handlers and try OS */
  NACL_SIGNAL_RETURN    /* Skip all other handlers and return */
};

/*
 * Prototype for a signal handler.  The handler will receive the POSIX
 * signal number and an opaque platform dependent signal object.
 */
typedef enum NaClSignalResult (*NaClSignalHandler)(int sig_num, void *ctx);

void NaClSignalStackRegister(void *stack);

/* with 0 signals handling is disabled */
void SetSignalHandling(int support_signals);

/*
 * Register process-wide signal handlers.
 */
void NaClSignalHandlerInit(void);

/* Undoes the effect of NaClSignalHandlerInit() */
void NaClSignalHandlerFini(void);

/* Assert that no signal handlers are registered */
void NaClSignalAssertNoHandlers(void);

/* Provides a signal safe method to write to stderr */
ssize_t NaClSignalErrorMessage(const char *str);

/*
 * Add a signal handler to the front of the list.
 * Returns an id for the handler or returns 0 on failure.
 * This function is not thread-safe and should only be
 * called at startup.
 */
int NaClSignalHandlerAdd(NaClSignalHandler func);

/*
 * Fill a signal context structure from the raw platform dependent
 * signal information.
 */
void NaClSignalContextFromHandler(struct NaClSignalContext *sigCtx,
                                  const void *rawCtx);

/*
 * Return non-zero if the signal context is currently executing in an
 * untrusted environment.
 */
int NaClSignalContextIsUntrusted(const struct NaClSignalContext *sigCtx);

/*
 * A basic handler which will exit with -signal_number when
 * a signal is encountered.
 */
enum NaClSignalResult NaClSignalHandleAll(int signal_number, void *ctx);

/*
 * Traverse handler list, until a handler returns
 * NACL_SIGNAL_RETURN, or the list is exhausted, in which case
 * the signal is passed to the OS.
 */
enum NaClSignalResult NaClSignalHandlerFind(int signal_number, void *ctx);

/* Platform specific code. Do not call directly */
void NaClSignalHandlerInitPlatform(void);
void NaClSignalHandlerFiniPlatform(void);

EXTERN_C_END

#endif  /* NACL_SIGNAL_H__ */
