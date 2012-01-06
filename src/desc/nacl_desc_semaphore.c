/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Semaphore Descriptor / Handle abstraction.
 */

#include <string.h>

#include "src/platform/nacl_host_desc.h"
#include "src/platform/nacl_log.h"
#include "src/desc/nacl_desc_semaphore.h"
#include "src/service_runtime/include/sys/errno.h"
#include "src/service_runtime/include/sys/stat.h"

/*
 * This file contains the implementation for the NaClDescSemaphore subclass
 * of NaClDesc.
 *
 * NaClDescSemaphore is the subclass that wraps host-OS semaphore abstractions
 */

static struct NaClDescVtbl const kNaClDescSemaphoreVtbl;  /* fwd */

void NaClDescSemaphoreDtor(struct NaClRefCount *vself) {
  struct NaClDescSemaphore *self = (struct NaClDescSemaphore *) vself;

  NaClLog(4, "NaClDescSemaphoreDtor(0x%08"NACL_PRIxPTR").\n",
          (uintptr_t) vself);
  NaClSemDtor(&self->sem);
  vself->vtbl = (struct NaClRefCountVtbl const *) &kNaClDescVtbl;
  (*vself->vtbl->Dtor)(vself);
}

int NaClDescSemaphoreFstat(struct NaClDesc          *vself,
                           struct nacl_abi_stat     *statbuf) {
  UNREFERENCED_PARAMETER(vself);

  memset(statbuf, 0, sizeof *statbuf);
  statbuf->nacl_abi_st_mode = NACL_ABI_S_IFSEMA;
  return 0;
}

int NaClDescSemaphorePost(struct NaClDesc         *vself) {
  struct NaClDescSemaphore *self = (struct NaClDescSemaphore *)vself;
  NaClSyncStatus status = NaClSemPost(&self->sem);

  return -NaClXlateNaClSyncStatus(status);
}

int NaClDescSemaphoreSemWait(struct NaClDesc          *vself) {
  struct NaClDescSemaphore *self = (struct NaClDescSemaphore *)vself;
  NaClSyncStatus status = NaClSemWait(&self->sem);

  return -NaClXlateNaClSyncStatus(status);
}

int NaClDescSemaphoreGetValue(struct NaClDesc         *vself) {
  UNREFERENCED_PARAMETER(vself);

  NaClLog(LOG_ERROR, "SemGetValue is not implemented yet\n");
  return -NACL_ABI_EINVAL;
  /*
   * TODO(gregoryd): sem_getvalue is not implemented on OSX.
   * Remove this syscall or implement it using semctl
   */
}


static struct NaClDescVtbl const kNaClDescSemaphoreVtbl = {
  {
    (void (*)(struct NaClRefCount *)) NaClDescSemaphoreDtor,
  },
  NaClDescMapNotImplemented,
  NaClDescUnmapUnsafeNotImplemented,
  NaClDescUnmapNotImplemented,
  NaClDescReadNotImplemented,
  NaClDescWriteNotImplemented,
  NaClDescSeekNotImplemented,
  NaClDescIoctlNotImplemented,
  NaClDescSemaphoreFstat,
  NaClDescGetdentsNotImplemented,
  NACL_DESC_SEMAPHORE,
  NaClDescExternalizeSizeNotImplemented,
  NaClDescExternalizeNotImplemented,
  NaClDescLockNotImplemented,
  NaClDescTryLockNotImplemented,
  NaClDescUnlockNotImplemented,
  NaClDescWaitNotImplemented,
  NaClDescTimedWaitAbsNotImplemented,
  NaClDescSignalNotImplemented,
  NaClDescBroadcastNotImplemented,
  NaClDescSendMsgNotImplemented,
  NaClDescRecvMsgNotImplemented,
  NaClDescConnectAddrNotImplemented,
  NaClDescAcceptConnNotImplemented,
  NaClDescSemaphorePost,
  NaClDescSemaphoreSemWait,
  NaClDescSemaphoreGetValue,
};
