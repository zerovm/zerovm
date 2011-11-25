/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Gather ye all module initializations and finalizations here.
 */
#include "src/desc/nrd_all_modules.h"
#include "src/fault_injection/fault_injection.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_syscall_handlers.h"
#include "src/service_runtime/nacl_thread_nice.h"
#include "src/service_runtime/nacl_tls.h"
#include "src/service_runtime/nacl_stack_safety.h"

void  NaClAllModulesInit(void) {
  NaClNrdAllModulesInit();
  NaClFaultInjectionModuleInit();
  NaClGlobalModuleInit();  /* various global variables */
  NaClStackSafetyInit();
  NaClTlsInit();
  /* d'b: disable most of syscalls. decision of initialization
   * will be done after command line parsing in "sel_main.c"
   */
  NaClSyscallTableInitDisable();
  /* d'b end */

  NaClThreadNiceInit();
}


void NaClAllModulesFini(void) {
  NaClNrdAllModulesFini();
  NaClTlsFini();
  NaClStackSafetyFini();
  NaClGlobalModuleFini();
}
