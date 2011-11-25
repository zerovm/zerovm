/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime user thread state.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_APP_THREAD_H__
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_APP_THREAD_H__ 1

#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"


EXTERN_C_BEGIN

struct NaClApp;

enum NaClThreadState {
  NACL_APP_THREAD_ALIVE,
  /* NACL_APP_THREAD_INTERRUPTIBLE_MUTEX, etc */
  NACL_APP_THREAD_SUICIDE_PENDING,
  NACL_APP_THREAD_DEAD
};

/*
 * Generally, only the thread itself will need to manipulate this
 * structure, but occasionally we may need to blow away a thread for
 * some reason, and look inside.  While a thread is in the NaCl
 * application running untrusted code, the lock must *not* be held.
 */
struct NaClAppThread {
  struct NaClMutex          mu;
  struct NaClCondVar        cv;

  uint32_t                  sysret;

  /*
   * The NaCl app that contains this thread.  The app must exist as
   * long as a thread is still alive.
   */
  struct NaClApp            *nap;

  int                       thread_num;  /* index into nap->threads */
  /*
   * sys_tls and tls2 are TLS values used by user code and the
   * integrated runtime (IRT) respectively.  The first TLS area may be
   * accessed via the %gs segment register on x86-32 so must point
   * into untrusted address space; we store it as a system pointer.
   * The second TLS may be an arbitrary value.
   */
  uintptr_t                 sys_tls;  /* first saved TLS ptr */
  uint32_t                  tls2;     /* second saved TLS value */

  struct NaClThread         thread;  /* low level thread representation */

  /*
   * a thread cannot free up its own mutex lock and commit suicide,
   * since another thread may be trying to summarily kill it and is
   * waiting on the lock in order to ask it to commit suicide!
   * instead, the suiciding thread just marks itself as dead, and a
   * periodic thread grabs a global thread table lock to do thread
   * deletion (which the thread requesting summary execution must also
   * grab).
   */
  enum NaClThreadState      state;

  struct NaClThreadContext  user;
  /* sys is only used to hold segment registers */
  struct NaClThreadContext  sys;
  /*
   * NaClThread abstraction allows us to specify the stack size
   * (NACL_KERN_STACK_SIZE), but not its location.  The underlying
   * implementation takes care of finding memory for the thread stack,
   * and when the thread exits (they're not joinable), the stack
   * should be automatically released.
   */

  uintptr_t                 *syscall_args;
  /*
   * user's sp translated to system address, used for accessing syscall
   * arguments
   */

  /* Stack for signal handling, registered with sigaltstack(). */
  void                      *signal_stack;


  /*
   * The last generation this thread reported into the service runtime
   * Protected by mu
   */
  int dynamic_delete_generation;
};

void NaClAppThreadTeardown(struct NaClAppThread *natp);

int NaClAppThreadCtor(struct NaClAppThread  *natp,
                      struct NaClApp        *nap,
                      uintptr_t             entry,
                      uintptr_t             stack_ptr,
                      uint32_t              tls_idx,
                      uintptr_t             sys_tls,
                      uint32_t              usr_tls2) NACL_WUR;

void NaClAppThreadDtor(struct NaClAppThread *natp);

/*
 * Low level initialization of thread, with validated values.  The
 * usr_entry and usr_stack_ptr values are directly used to initialize the
 * user register values; the sys_tls_base is the system address for
 * allocating a %gs thread descriptor block base.  The caller is
 * responsible for error checking: usr_entry is a valid entry point (0
 * mod N) and sys_tls_base is in the NaClApp's address space.
 */
int NaClAppThreadAllocSegCtor(struct NaClAppThread  *natp,
                              struct NaClApp        *nap,
                              uintptr_t             usr_entry,
                              uintptr_t             usr_stack_ptr,
                              uintptr_t             sys_tls_base,
                              uint32_t              usr_tls2) NACL_WUR;

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SERVICE_RUNTIME_NACL_APP_THREAD_H__ */
