/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime mutex and condition variable abstraction layer.
 * The NaClX* interfaces just invoke the no-X versions of the
 * synchronization routines, and aborts if there are any error
 * returns.
 */

#include "src/platform/nacl_log.h"
#include "src/platform/nacl_sync_checked.h"

void NaClXMutexCtor(struct NaClMutex *mp) {
  if (!NaClMutexCtor(mp)) {
    NaClLog(LOG_FATAL, "NaClXMutexCtor failed\n");
  }
}

void NaClXMutexLock(struct NaClMutex *mp) {
  NaClSyncStatus  status;

  if (NACL_SYNC_OK == (status = NaClMutexLock(mp))) {
    return;
  }
  NaClLog(LOG_FATAL, "NaClMutexLock returned %d\n", status);
}

void NaClXMutexUnlock(struct NaClMutex *mp) {
  NaClSyncStatus  status;

  if (NACL_SYNC_OK == (status = NaClMutexUnlock(mp))) {
    return;
  }
  NaClLog(LOG_FATAL, "NaClMutexUnlock returned %d\n", status);
}

// ###
//void NaClXCondVarSignal(struct NaClCondVar *cvp) {
//  NaClSyncStatus  status;
//
//  if (NACL_SYNC_OK == (status = NaClCondVarSignal(cvp))) {
//    return;
//  }
//  NaClLog(LOG_FATAL, "NaClCondVarSignal returned %d\n", status);
//}

// ###
//void NaClXCondVarBroadcast(struct NaClCondVar *cvp) {
//  NaClSyncStatus  status;
//
//  if (NACL_SYNC_OK == (status = NaClCondVarBroadcast(cvp))) {
//    return;
//  }
//  NaClLog(LOG_FATAL, "NaClCondVarBroadcast returned %d\n", status);
//}

// ###
//void NaClXCondVarWait(struct NaClCondVar *cvp,
//                      struct NaClMutex   *mp) {
//  NaClSyncStatus  status;
//
//  if (NACL_SYNC_OK == (status = NaClCondVarWait(cvp, mp))) {
//    return;
//  }
//  NaClLog(LOG_FATAL, "NaClCondVarWait returned %d\n", status);
//}
