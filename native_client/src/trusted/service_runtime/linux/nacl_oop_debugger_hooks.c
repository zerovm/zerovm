/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/trusted/service_runtime/nacl_oop_debugger_hooks.h"

/* placeholders */
void NaClOopDebuggerAppCreateHook(struct NaClApp *nap) {
  UNREFERENCED_PARAMETER(nap);
}

void NaClOopDebuggerThreadCreateHook(struct NaClAppThread *natp) {
  UNREFERENCED_PARAMETER(natp);
}
void NaClOopDebuggerThreadExitHook(struct NaClAppThread *natp,
                                   int exit_code) {
  UNREFERENCED_PARAMETER(natp);
  UNREFERENCED_PARAMETER(exit_code);
}

void NaClOopDebuggerAppExitHook(int exit_code) {
  UNREFERENCED_PARAMETER(exit_code);
}

