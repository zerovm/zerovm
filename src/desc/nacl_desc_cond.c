/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Condition Variable Descriptor / Handle abstraction.
 */

#include <string.h>

#include "src/desc/nacl_desc_cond.h"
#include "src/platform/nacl_log.h"
#include "src/service_runtime/include/sys/errno.h"
#include "src/service_runtime/include/sys/stat.h"

// ###
///*
// * This file contains the implementation for the NaClDescCondVar subclass
// * of NaClDesc.
// *
// * NaClDescCondVar is the subclass that wraps host-OS condition
// * variable abstractions
// */
//
//static struct NaClDescVtbl const kNaClDescCondVarVtbl;  /* fwd */

// ###
//static void NaClDescCondVarDtor(struct NaClRefCount *vself) {
//  struct NaClDescCondVar *self = (struct NaClDescCondVar *) vself;
//
//  NaClLog(4, "NaClDescCondVarDtor(0x%08"NACL_PRIxPTR").\n",
//          (uintptr_t) vself);
//  NaClIntrCondVarDtor(&self->cv);
//
//  vself->vtbl = (struct NaClRefCountVtbl const *) &kNaClDescVtbl;
//  (*vself->vtbl->Dtor)(vself);
//}

// ###
//static int NaClDescCondVarFstat(struct NaClDesc          *vself,
//                                struct nacl_abi_stat     *statbuf) {
//  UNREFERENCED_PARAMETER(vself);
//
//  memset(statbuf, 0, sizeof *statbuf);
//  statbuf->nacl_abi_st_mode = NACL_ABI_S_IFCOND | NACL_ABI_S_IRWXU;
//  return 0;
//}

// ###
//static int NaClDescCondVarWait(struct NaClDesc         *vself,
//                               struct NaClDesc         *mutex) {
//  struct NaClDescCondVar  *self = (struct NaClDescCondVar *) vself;
//  struct NaClDescMutex    *mutex_desc;
//  NaClSyncStatus          status;
//
//  if (((struct NaClDescVtbl const *) mutex->base.vtbl)->
//      typeTag != NACL_DESC_MUTEX) {
//    return -NACL_ABI_EINVAL;
//  }
//  mutex_desc = (struct NaClDescMutex *)mutex;
//  status = NaClIntrCondVarWait(&self->cv,
//                               &mutex_desc->mu,
//                               NULL);
//  return -NaClXlateNaClSyncStatus(status);
//}

// ###
//static int NaClDescCondVarTimedWaitAbs(struct NaClDesc                *vself,
//                                       struct NaClDesc                *mutex,
//                                       struct nacl_abi_timespec const *ts) {
//  struct NaClDescCondVar  *self = (struct NaClDescCondVar *) vself;
//  struct NaClDescMutex    *mutex_desc;
//  NaClSyncStatus          status;
//
//  if (((struct NaClDescVtbl const *) mutex->base.vtbl)->
//      typeTag != NACL_DESC_MUTEX) {
//    return -NACL_ABI_EINVAL;
//  }
//  mutex_desc = (struct NaClDescMutex *) mutex;
//
//  status = NaClIntrCondVarWait(&self->cv,
//                               &mutex_desc->mu,
//                               ts);
//  return -NaClXlateNaClSyncStatus(status);
//}

// ###
//static int NaClDescCondVarSignal(struct NaClDesc         *vself) {
//  struct NaClDescCondVar *self = (struct NaClDescCondVar *) vself;
//  NaClSyncStatus status = NaClIntrCondVarSignal(&self->cv);
//
//  return -NaClXlateNaClSyncStatus(status);
//}

// ###
//static int NaClDescCondVarBroadcast(struct NaClDesc          *vself) {
//  struct NaClDescCondVar *self = (struct NaClDescCondVar *) vself;
//  NaClSyncStatus status = NaClIntrCondVarBroadcast(&self->cv);
//
//  return -NaClXlateNaClSyncStatus(status);
//}

// ###
//static struct NaClDescVtbl const kNaClDescCondVarVtbl = {
//  {
//    NaClDescCondVarDtor,
//  },
//  NaClDescMapNotImplemented,
//  NaClDescUnmapUnsafeNotImplemented,
//  NaClDescUnmapNotImplemented,
//  NaClDescReadNotImplemented,
//  NaClDescWriteNotImplemented,
//  NaClDescSeekNotImplemented,
//  NaClDescIoctlNotImplemented,
//  NaClDescCondVarFstat,
//  NaClDescGetdentsNotImplemented,
//  NACL_DESC_CONDVAR,
//  NaClDescExternalizeSizeNotImplemented,
//  NaClDescExternalizeNotImplemented,
//  NaClDescLockNotImplemented,
//  NaClDescTryLockNotImplemented,
//  NaClDescUnlockNotImplemented,
//  NaClDescCondVarWait,
//  NaClDescCondVarTimedWaitAbs,
//  NaClDescCondVarSignal,
//  NaClDescCondVarBroadcast,
//  NaClDescSendMsgNotImplemented,
//  NaClDescRecvMsgNotImplemented,
//  NaClDescConnectAddrNotImplemented,
//  NaClDescAcceptConnNotImplemented,
//  NaClDescPostNotImplemented,
//  NaClDescSemWaitNotImplemented,
//  NaClDescGetValueNotImplemented,
//};
