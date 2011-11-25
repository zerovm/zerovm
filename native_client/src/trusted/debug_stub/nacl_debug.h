/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/*
 * This module provides a 'C' style interface to the NaCl debugging
 * related functions, and drives the C++ debugging stub.  The functions
 * defined here are safe to call, because they always check if debugging
 * is enabled and catch all exceptions throws by underlying C++ libraries.
 * For this reason, the debug stub library should not be called directly.
 *
 * The behavior of the debug stub is controlled through a combination of
 * a set of environment (described bellow), and conditional compilation
 * based on definition of _DEBUG.
 *
 * These functions are non-reentrant except for the functions notifying
 * the debug stub that thread has been created or is about to be
 * destroyed.  It is expected the other functions will all be called by
 * the main thread.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_DEBUG_STUB_NACL_DEBUG_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DEBUG_STUB_NACL_DEBUG_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

struct NaClApp;
struct NaClAppThread;

/*
 * These functions should be called before the debug server has started
 * to prevent a debugger from attaching and failing to retrieve
 * information about the App (NEXE) that is running.  These functions are
 * safe to call even if debugging has not been enabled.
 */
void NaClDebugSetAppInfo(struct NaClApp *app) NO_THROW;
void NaClDebugSetAppEnvironment(int argc, char const * const argv[],
                                int envc, char const * const envv[]) NO_THROW;

/*
 * This function must be called each time we start a new App thread by the
 * by the untrusted thread itself as early as possible.  The function will
 * notifying the debuging stub of it's existance, and preparing the thread
 * for signal or exception handling.  For each thread that calls this
 * function, if debugging is:
 *
 *   ENABLED : all exceptions will be sent to the debug stub.
 *   DISABLED: all exceptions will be sent to breakpad.
 */
void NaClDebugThreadPrepDebugging(struct NaClAppThread *natp) NO_THROW;


/*
 * This function notifies the debug stub that the provided thread
 * should no longer be debugged.  This is typically because the thread
 * is about to halt.  TODO(noelallen) Unlike "Prep", this function can
 * be called by any thread.
 */
void NaClDebugThreadStopDebugging(struct NaClAppThread *natp) NO_THROW;


/*
 * This function notifies the debug stub that the system is ready to
 * allow debugging, and should finish preparation for debuggers to attach.
 * The function will launch a new thread to act as a sever processing GDB
 * RSP based connections.  How a remote debugger connects to the debugging
 * stub is controlled by the following environment variables:
 *   NACL_DEBUG_ENABLE - if defined, debugging is allowed
 *   NACL_DEBUG_IP - IPv4 address on which to bind (default "127.0.0.1")
 *   NACL_DEBUG_PORT - Port(s) on which to listen (default "8000:8010")
 */
int NaClDebugStart(void) NO_THROW;


/*
 * Signals the debugging thread (if one has started), that it should
 * release all debugging resources and halt.  Prior to closing any
 * connections, the debugging stub has the opportunity to notify
 * an attached debugger (assuming it is in a state to receive the
 * message) of the App exit code.
 */
void NaClDebugStop(int exitCode) NO_THROW;


EXTERN_C_END

#endif  /* NATIVE_CLIENT_SERVICE_RUNTIME_NACL_DEBUG_H_ */
