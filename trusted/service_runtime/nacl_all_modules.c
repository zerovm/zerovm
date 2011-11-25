/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Gather ye all module initializations and finalizations here.
 */
#include "shared/srpc/nacl_srpc.h"
#include "trusted/debug_stub/debug_stub.h"
#include "trusted/desc/nrd_all_modules.h"
#include "trusted/fault_injection/fault_injection.h"
#include "trusted/handle_pass/ldr_handle.h"
#include "trusted/service_runtime/nacl_globals.h"
#include "trusted/service_runtime/nacl_syscall_handlers.h"
#include "trusted/service_runtime/nacl_thread_nice.h"
#include "trusted/service_runtime/nacl_tls.h"
#include "trusted/service_runtime/nacl_stack_safety.h"

void  NaClAllModulesInit(void) {
  NaClNrdAllModulesInit();
  NaClFaultInjectionModuleInit();
  NaClGlobalModuleInit();  /* various global variables */
  NaClStackSafetyInit();
  NaClSrpcModuleInit();
  NaClTlsInit();
  /* d'b: disable most of syscalls. decision of initialization
   * will be done after command line parsing in "sel_main.c"
   */
  NaClSyscallTableInitDisable();
  /* d'b end */

  NaClThreadNiceInit();
#if NACL_WINDOWS && !defined(NACL_STANDALONE)
  NaClHandlePassLdrInit();
#endif
}


void NaClAllModulesFini(void) {
  NaClNrdAllModulesFini();
  NaClTlsFini();
  NaClSrpcModuleFini();
  NaClStackSafetyFini();
  NaClGlobalModuleFini();
}
