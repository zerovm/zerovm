/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime global scoped objects for handling global resources.
 */

#include "src/platform/nacl_sync_checked.h"
#include "src/service_runtime/nacl_globals.h"

struct NaClMutex            nacl_thread_mu;
struct NaClThreadContext    *nacl_user = NULL; /* d'b: object to temporary hold user registers */
struct NaClThreadContext    *nacl_sys = NULL; /* d'b: object to hold zvm registers while control is passed to nexe */
int64_t                     syscallback = 0; /* d'b */
jmp_buf                     user_exit; /* d'b: for user trap() exit */
struct NaClApp              *gnap = NULL; /* d'b: global NaClApp object. could be removed later or
                                     be a replacement for same object constructed from main() */

/*
 * Hack for gdb.  This records xlate_base in a place where (1) gdb can find it,
 * and (2) gdb doesn't need debug info (it just needs symbol info).
 */
uintptr_t                   nacl_global_xlate_base;

void NaClGlobalModuleInit(void) {
  NaClXMutexCtor(&nacl_thread_mu);
}

void  NaClGlobalModuleFini(void) {
  NaClMutexDtor(&nacl_thread_mu);
}
