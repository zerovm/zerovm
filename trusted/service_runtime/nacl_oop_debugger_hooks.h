/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SRC_TRUSTED_SERVICE_RUNTIME_NACL_OOP_DEBUGGER_HOOKS_H_
#define SRC_TRUSTED_SERVICE_RUNTIME_NACL_OOP_DEBUGGER_HOOKS_H_

struct NaClApp;
struct NaClAppThread;

void NaClOopDebuggerAppCreateHook(struct NaClApp *nap);
void NaClOopDebuggerThreadCreateHook(struct NaClAppThread *natp);
void NaClOopDebuggerThreadExitHook(struct NaClAppThread *natp, int exit_code);
void NaClOopDebuggerAppExitHook(int exit_code);

#endif  // SRC_TRUSTED_SERVICE_RUNTIME_NACL_OOP_DEBUGGER_HOOKS_H_

