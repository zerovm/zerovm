/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Gather ye all module initializations and finalizations here.
 */
#include "src/desc/nrd_all_modules.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_syscall_handlers.h"

void NaClAllModulesInit(void)
{
  NaClNrdAllModulesInit();
  NaClSyscallTableInit();
}

void NaClAllModulesFini(void)
{
  NaClNrdAllModulesFini();
}
