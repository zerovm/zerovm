/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SIGNAL_H__
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SIGNAL_H__ 1

/*
 * The nacl_signal module provides a platform independent mechanism for
 * trapping signals encountered while running a Native Client executable.
 * Signal handlers can be installed which will receive a POSIX signal number
 * and a platform dependent signal object.  Accessors are provided to convert
 * to and from architecture dependent CPU state structures.
 */

#include "src/service_runtime/arch/x86_64/nacl_signal_64.h"

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

/*
 * Allocates a stack suitable for passing to
 * NaClSignalStackRegister(), for use as a stack for signal handlers.
 * This can be called in any thread.
 * Stores the result in *result; returns 1 on success, 0 on failure.
 */
int NaClSignalStackAllocate(void **result);

/*
 * Register process-wide signal handlers.
 */
void NaClSignalHandlerInit(void);

/*
 * Undoes the effect of NaClSignalHandlerInit().
 */
void NaClSignalHandlerFini(void);

/*
 * Assert that no signal handlers are registered.
 */
void NaClSignalAssertNoHandlers(void);

/*
 * Provides a signal safe method to write to stderr.
 */
ssize_t NaClSignalErrorMessage(const char *str);

/*
 * Add a signal handler to the front of the list.
 * Returns an id for the handler or returns 0 on failure.
 * This function is not thread-safe and should only be
 * called at startup.
 */
int NaClSignalHandlerAdd(NaClSignalHandler func);

/*
 * Remove a signal handler based on the ID provided, and
 * return 1 on success or zero on failure.  This function
 * is not thread-safe and should only be called at startup.
 */
int NaClSignalHandlerRemove(int id);

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
 * A basic handler which will do nothing, passing the
 * error to the OS.
 */
enum NaClSignalResult NaClSignalHandleNone(int signal_number, void *ctx);

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

/*
 * Platform specific code. Do not call directly.
 */
void NaClSignalHandlerInitPlatform(void);
void NaClSignalHandlerFiniPlatform(void);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SIGNAL_H__ */
