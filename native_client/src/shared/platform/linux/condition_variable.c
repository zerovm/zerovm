/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <pthread.h>
#include <sys/time.h>
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/include/atomic_ops.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/trusted/service_runtime/include/sys/time.h"

static const uint64_t kMicrosecondsPerSecond = 1000 * 1000;
static const uint64_t kNanosecondsPerMicrosecond = 1000;

#if NACL_COND_BROADCAST_HACK
#define ATOMIC_INC(_var)  (AtomicIncrement((_var), (Atomic32)1))
#define ATOMIC_DEC(_var)  (AtomicIncrement((_var), (Atomic32)-1))
#endif

/* Condition variable C API */

int NaClCondVarCtor(struct NaClCondVar  *cvp) {
  if (0 != pthread_cond_init(&cvp->cv, (pthread_condattr_t *) 0)) {
    return 0;
  }
#if NACL_COND_BROADCAST_HACK
  cvp->waiting = 0;
#endif
  return 1;
}

void NaClCondVarDtor(struct NaClCondVar *cvp) {
  pthread_cond_destroy(&cvp->cv);
}

NaClSyncStatus NaClCondVarSignal(struct NaClCondVar *cvp) {
  pthread_cond_signal(&cvp->cv);
  return NACL_SYNC_OK;
}

#if NACL_COND_BROADCAST_HACK
/* Strange linking bug:
 * If pthread_cond_broadcast is not referenced in this file, then
 * the function is never linked in, but libstdc++ tries to use it
 * anyway, resulting in an exception being thrown.
 * BUG= http://code.google.com/p/nativeclient/issues/detail?id=1987
 */
int (*fake_reference_to_pthread_cond_broadcast)(pthread_cond_t*) =
    pthread_cond_broadcast;
#endif

NaClSyncStatus NaClCondVarBroadcast(struct NaClCondVar *cvp) {
#if NACL_COND_BROADCAST_HACK
  /* There is a race condition where cvp->waiting may be
   * incremented before a thread actually calls pthread_cond_(timed)wait.
   * In that case, we'll send an extra signal and it'll go nowhere.
   * This is not a problem, because broadcasting always
   * has this race condition. If both the broadcast and
   * wait are protected by the same mutex, then the waiting counter
   * will be accurate
   */
  uint32_t waiting = cvp->waiting;
  while (waiting > 0) {
    pthread_cond_signal(&cvp->cv);
    waiting--;
  }
#else
  pthread_cond_broadcast(&cvp->cv);
#endif
  return NACL_SYNC_OK;
}

NaClSyncStatus NaClCondVarWait(struct NaClCondVar *cvp,
                               struct NaClMutex   *mp) {
#if NACL_COND_BROADCAST_HACK
  ATOMIC_INC(&cvp->waiting);
#endif
  pthread_cond_wait(&cvp->cv, &mp->mu);
#if NACL_COND_BROADCAST_HACK
  ATOMIC_DEC(&cvp->waiting);
#endif
  return NACL_SYNC_OK;
}

NaClSyncStatus NaClCondVarTimedWaitRelative(
    struct NaClCondVar             *cvp,
    struct NaClMutex               *mp,
    NACL_TIMESPEC_T const          *reltime) {
  uint64_t relative_wait_us =
      reltime->tv_sec * kMicrosecondsPerSecond +
      reltime->tv_nsec / kNanosecondsPerMicrosecond;
  uint64_t current_time_us;
  uint64_t wakeup_time_us;
  int result;
  struct timespec ts;
  struct timeval tv;
  struct timezone tz = { 0, 0 };  /* UTC */
  if (gettimeofday(&tv, &tz) == 0) {
    current_time_us = tv.tv_sec * kMicrosecondsPerSecond + tv.tv_usec;
  } else {
    return NACL_SYNC_INTERNAL_ERROR;
  }
  wakeup_time_us = current_time_us + relative_wait_us;
  ts.tv_sec = wakeup_time_us / kMicrosecondsPerSecond;
  ts.tv_nsec = (wakeup_time_us - ts.tv_sec * kMicrosecondsPerSecond) *
               kNanosecondsPerMicrosecond;

#if NACL_COND_BROADCAST_HACK
  ATOMIC_INC(&cvp->waiting);
#endif
  result = pthread_cond_timedwait(&cvp->cv, &mp->mu, &ts);
#if NACL_COND_BROADCAST_HACK
  ATOMIC_DEC(&cvp->waiting);
#endif
  if (0 == result) {
    return NACL_SYNC_OK;
  }
  return NACL_SYNC_CONDVAR_TIMEDOUT;
}

NaClSyncStatus NaClCondVarTimedWaitAbsolute(
    struct NaClCondVar              *cvp,
    struct NaClMutex                *mp,
    NACL_TIMESPEC_T const           *abstime) {
  struct timespec ts;
  int result;
  ts.tv_sec = abstime->tv_sec;
  ts.tv_nsec = abstime->tv_nsec;
#if NACL_COND_BROADCAST_HACK
  ATOMIC_INC(&cvp->waiting);
#endif
  result = pthread_cond_timedwait(&cvp->cv, &mp->mu, &ts);
#if NACL_COND_BROADCAST_HACK
  ATOMIC_DEC(&cvp->waiting);
#endif
  if (0 == result) {
    return NACL_SYNC_OK;
  }
  return NACL_SYNC_CONDVAR_TIMEDOUT;
}
