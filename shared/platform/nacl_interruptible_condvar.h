/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Server Runtime interruptible condvar, based on nacl_sync
 * interface. We need a condvar that can handle interruptible mutexes
 * correctly.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_INTERRUPTIBLE_CONDVAR_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_INTERRUPTIBLE_CONDVAR_H_

#include "include/nacl_base.h"
#include "shared/platform/nacl_interruptible_mutex.h"
#include "shared/platform/nacl_sync.h"
#include "trusted/service_runtime/include/sys/time.h"


EXTERN_C_BEGIN


struct NaClIntrCondVar {
  struct NaClCondVar  cv;
};

int NaClIntrCondVarCtor(struct NaClIntrCondVar  *cp);

void NaClIntrCondVarDtor(struct NaClIntrCondVar *cp);

NaClSyncStatus NaClIntrCondVarWait(struct NaClIntrCondVar         *cp,
                                   struct NaClIntrMutex           *mp,
                                   struct nacl_abi_timespec const *ts);

NaClSyncStatus NaClIntrCondVarSignal(struct NaClIntrCondVar *cp);

NaClSyncStatus NaClIntrCondVarBroadcast(struct NaClIntrCondVar *cp);

void NaClIntrCondVarIntr(struct NaClIntrCondVar  *cp);

void NaClIntrCondVarReset(struct NaClIntrCondVar *cp);

EXTERN_C_END


#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_INTERRUPTIBLE_CONDVAR_H_ */
