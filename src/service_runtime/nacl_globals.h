/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Server Runtime globals.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_GLOBALS_H__
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_GLOBALS_H__

#include <setjmp.h> /* d'b: need for trap() exit */

EXTERN_C_BEGIN
struct NaClThreadContext;
struct NaClAppThread;
struct NaClMutex;
struct NaClApp;

extern int64_t syscallback; /* d'b */
struct NaClThreadContext    *nacl_user; /* d'b */
struct NaClThreadContext    *nacl_sys; /* d'b */
struct NaClApp              *gnap; /* d'b */
jmp_buf                     user_exit; /* d'b */

extern struct NaClMutex         nacl_thread_mu;
/*
 * nacl_thread_mu protects access to nacl_thread, which are
 * dynamically allocated (kernel stacks are too large to keep around
 * one for each potential thread).  once the pointer is accessed and
 * the object's lock held, nacl_thread_mu may be dropped.
 */

void  NaClGlobalModuleInit(void);
void  NaClGlobalModuleFini(void);

/* hack for gdb */
extern uintptr_t nacl_global_xlate_base;

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_GLOBALS_H__ */
