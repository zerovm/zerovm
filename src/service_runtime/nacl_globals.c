/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime global scoped objects for handling global resources.
 */

#include "src/platform/nacl_interruptible_mutex.h"
#include "src/platform/nacl_log.h"
#include "src/platform/nacl_sync.h"
#include "src/platform/nacl_sync_checked.h"
#include "src/platform/nacl_threads.h"
#include "src/service_runtime/arch/sel_ldr_arch.h"
#include "src/service_runtime/nacl_app.h"
#include "src/service_runtime/nacl_app_thread.h"
#include "src/service_runtime/nacl_globals.h"

struct NaClMutex            nacl_thread_mu;

struct NaClThreadContext    *nacl_user[NACL_THREAD_MAX] = {NULL};
struct NaClThreadContext    *nacl_sys[NACL_THREAD_MAX] = {NULL};
struct NaClAppThread        *nacl_thread[NACL_THREAD_MAX] = {NULL};

/*
 * Hack for gdb.  This records xlate_base in a place where (1) gdb can find it,
 * and (2) gdb doesn't need debug info (it just needs symbol info).
 */
uintptr_t                   nacl_global_xlate_base;

void NaClGlobalModuleInit(void) {
  NaClXMutexCtor(&nacl_thread_mu);
  NaClInitGlobals();
}


void  NaClGlobalModuleFini(void) {
  NaClMutexDtor(&nacl_thread_mu);
}
