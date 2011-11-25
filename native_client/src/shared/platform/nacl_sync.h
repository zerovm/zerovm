/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime mutex and condition variable abstraction layer.
 * This is the host-OS-independent interface.
 */
#ifndef NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_SYNC_H_
#define NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_SYNC_H_

#if defined(__native_client__) || NACL_LINUX || NACL_OSX
#include <pthread.h>
#endif
#include "native_client/src/include/atomic_ops.h"
#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/trusted/service_runtime/include/machine/_types.h"

EXTERN_C_BEGIN

// It is very difficult to get a definition of nacl_abi_timespec inside of a
// native client compile.  Sidestep this issue for now.
// TODO(bsy,sehr): change the include guards on time.h to allow both defines.
#ifdef __native_client__
// In a native client module timespec and nacl_abi_timespec are the same.
typedef struct timespec NACL_TIMESPEC_T;
#else
// In trusted code time.h is not derived from
// src/trusted/service_runtime/include/sys/time.h, so there is no conflict.
typedef struct nacl_abi_timespec NACL_TIMESPEC_T;
#endif

struct NaClMutex {
#if defined(__native_client__) || NACL_LINUX || NACL_OSX
  pthread_mutex_t mu;
#else
  void* lock;
#endif
};

/*
 * pthread_cond_broadcast seems to be broken on ARM QEMU.
 * It only wakes one thread up instead of all of them.
 * This is an ugly workaround.
 */
#if NACL_LINUX && !defined(__native_client__) && \
    NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
#define NACL_COND_BROADCAST_HACK   1
#else
#define NACL_COND_BROADCAST_HACK   0
#endif

struct NaClCondVar {
#if defined(__native_client__) || NACL_LINUX || NACL_OSX
  pthread_cond_t cv;
#if NACL_COND_BROADCAST_HACK
  Atomic32 waiting;
#endif
#else
  void* cv;
#endif
};

struct nacl_abi_timespec;

typedef enum NaClSyncStatus {
  NACL_SYNC_OK,
  NACL_SYNC_INTERNAL_ERROR,
  NACL_SYNC_BUSY,
  NACL_SYNC_MUTEX_INVALID,
  NACL_SYNC_MUTEX_DEADLOCK,
  NACL_SYNC_MUTEX_PERMISSION,
  NACL_SYNC_MUTEX_INTERRUPTED,
  NACL_SYNC_CONDVAR_TIMEDOUT,
  NACL_SYNC_CONDVAR_INTR,
  NACL_SYNC_SEM_INTERRUPTED,
  NACL_SYNC_SEM_RANGE_ERROR
} NaClSyncStatus;

int NaClMutexCtor(struct NaClMutex *mp) NACL_WUR;  /* bool success/fail */

void NaClMutexDtor(struct NaClMutex *mp);

NaClSyncStatus NaClMutexLock(struct NaClMutex *mp) NACL_WUR;

NaClSyncStatus NaClMutexTryLock(struct NaClMutex *mp) NACL_WUR;

NaClSyncStatus NaClMutexUnlock(struct NaClMutex *mp) NACL_WUR;


int NaClCondVarCtor(struct NaClCondVar *cvp) NACL_WUR;

void NaClCondVarDtor(struct NaClCondVar *cvp);

NaClSyncStatus NaClCondVarSignal(struct NaClCondVar *cvp) NACL_WUR;

NaClSyncStatus NaClCondVarBroadcast(struct NaClCondVar *cvp) NACL_WUR;

NaClSyncStatus NaClCondVarWait(struct NaClCondVar *cvp,
                               struct NaClMutex   *mp) NACL_WUR;

NaClSyncStatus NaClCondVarTimedWaitRelative(
    struct NaClCondVar              *cvp,
    struct NaClMutex                *mp,
    NACL_TIMESPEC_T const           *reltime) NACL_WUR;

NaClSyncStatus NaClCondVarTimedWaitAbsolute(
    struct NaClCondVar              *cvp,
    struct NaClMutex                *mp,
    NACL_TIMESPEC_T const           *abstime) NACL_WUR;



EXTERN_C_END


#endif  /* NATIVE_CLIENT_SRC_SHARED_PLATFORM_NACL_SYNC_H_ */
