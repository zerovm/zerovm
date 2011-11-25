/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime threads implementation layer.
 */

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include <limits.h>
/*
 * PTHREAD_STACK_MIN should come from pthread.h as documented, but is
 * actually pulled in by limits.h.
 */

#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

#if !defined(__native_client__) && NACL_KERN_STACK_SIZE < PTHREAD_STACK_MIN
# error "NaCl service runtime stack size is smaller than PTHREAD_STACK_MIN"
#endif

static int NaClThreadCreate(struct NaClThread  *ntp,
                            void               (*start_fn)(void *),
                            void               *state,
                            size_t             stack_size,
                            int                is_detached) {
  pthread_attr_t  attr;
  int             code;
  int             rv;
  char            err_string[1024];

  rv = 0;

  if (stack_size < PTHREAD_STACK_MIN) {
    stack_size = PTHREAD_STACK_MIN;
  }
  if (0 != (code = pthread_attr_init(&attr))) {
    NaClLog(LOG_ERROR,
            "NaClThreadCtor: pthread_atr_init returned %d",
            code);
    goto done;
  }
  if (0 != (code = pthread_attr_setstacksize(&attr, stack_size))) {
    NaClLog(LOG_ERROR,
            "NaClThreadCtor: pthread_attr_setstacksize returned %d (%s)",
            code,
            (strerror_r(code, err_string, sizeof err_string) == 0
             ? err_string
             : "UNKNOWN"));
    goto done_attr_dtor;
  }
  if (is_detached) {
    if (0 != (code = pthread_attr_setdetachstate(&attr,
                                                 PTHREAD_CREATE_DETACHED))) {
      NaClLog(LOG_ERROR,
              "nacl_thread: pthread_attr_setdetachstate returned %d (%s)",
              code,
              (strerror_r(code, err_string, sizeof err_string) == 0
               ? err_string
               : "UNKNOWN"));
      goto done_attr_dtor;
    }
  }
  if (0 != (code = pthread_create(&ntp->tid,
                                  &attr,
                                  (void *(*)(void *)) start_fn,
                                  state))) {
    NaClLog(LOG_ERROR,
            "nacl_thread: pthread_create returned %d (%s)",
            code,
            (strerror_r(code, err_string, sizeof err_string) == 0
             ? err_string
             : "UNKNOWN"));
    goto done_attr_dtor;
  }
  rv = 1;
 done_attr_dtor:
  (void) pthread_attr_destroy(&attr);  /* often a noop */
 done:
  return rv;
}

/*
 * Even if ctor fails, it should be okay -- and required -- to invoke
 * the dtor on it.
 */
int NaClThreadCtor(struct NaClThread  *ntp,
                   void               (*start_fn)(void *),
                   void               *state,
                   size_t             stack_size) {
  return NaClThreadCreate(ntp, start_fn, state, stack_size,
                          /* is_detached= */ 1);
}

int NaClThreadCreateJoinable(struct NaClThread  *ntp,
                             void               (*start_fn)(void *),
                             void               *state,
                             size_t             stack_size) {
  return NaClThreadCreate(ntp, start_fn, state, stack_size,
                          /* is_detached= */ 0);
}

void NaClThreadDtor(struct NaClThread *ntp) {
  /*
   * The threads that we create are not joinable, and we cannot tell
   * when they are truly gone.  Fortunately, the threads themselves
   * and the underlying thread library are responsible for ensuring
   * that resources such as the thread stack are properly released.
   */
  UNREFERENCED_PARAMETER(ntp);
}

void NaClThreadJoin(struct NaClThread *ntp) {
  pthread_join(ntp->tid, NULL);
}

void NaClThreadExit(int exit_code) {
  pthread_exit((void *) (uintptr_t) exit_code);
}
