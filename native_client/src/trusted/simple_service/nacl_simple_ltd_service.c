/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/simple_service/nacl_simple_ltd_service.h"

#include "native_client/src/include/nacl_compiler_annotations.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/shared/srpc/nacl_srpc.h"

int NaClSimpleLtdServiceCtor(
    struct NaClSimpleLtdService       *self,
    struct NaClSrpcHandlerDesc const  *srpc_handlers,
    int                               max_cli,
    NaClThreadIfFactoryFunction       thread_factory_fn,
    void                              *thread_factory_data) {
  NaClLog(4, "Entered NaClSimpleLtdServiceCtor\n");
  if (!NaClSimpleServiceCtor((struct NaClSimpleService *) self,
                             srpc_handlers,
                             thread_factory_fn,
                             thread_factory_data)) {
    NaClLog(4, "NaClSimpleServiceCtor failed\n");
    goto base_ctor_fail;
  }
  if (!NaClMutexCtor(&self->mu)) {
    NaClLog(4, "NaClSimpleLtdServiceCtor: NaClMutexCtor failed\n");
    goto mutex_ctor_fail;
  }
  if (!NaClCondVarCtor(&self->cv)) {
    NaClLog(4, "NaClSimpleLtdServiceCtor: NaClCondVarCtor failed\n");
    goto condvar_ctor_fail;
  }
  self->max_clients = max_cli;
  self->num_clients = 0;
  NACL_VTBL(NaClRefCount, self) =
      (struct NaClRefCountVtbl *) &kNaClSimpleLtdServiceVtbl;
  NaClLog(4, "NaClSimpleLtdServiceCtor: success\n");
  return 1;

  /* failure cascade, in reverse construction order */
 condvar_ctor_fail:
  NaClMutexDtor(&self->mu);
 mutex_ctor_fail:
  (*NACL_VTBL(NaClRefCount, self)->Dtor)((struct NaClRefCount *) self);
 base_ctor_fail:
  return 0;
}

void NaClSimpleLtdServiceDtor(struct NaClRefCount *vself) {
  struct NaClSimpleLtdService *self =
      (struct NaClSimpleLtdService *) vself;

  CHECK(0 == self->num_clients);
  NaClCondVarDtor(&self->cv);
  NaClMutexDtor(&self->mu);
  NACL_VTBL(NaClRefCount, self) =
      (struct NaClRefCountVtbl *) &kNaClSimpleServiceVtbl;
  (*NACL_VTBL(NaClRefCount, self)->Dtor)((struct NaClRefCount *) self);
}


int NaClSimpleLtdServiceAcceptConnection(
    struct NaClSimpleService            *vself,
    struct NaClSimpleServiceConnection  **out) {
  struct NaClSimpleLtdService *self =
      (struct NaClSimpleLtdService *) vself;
  int                         rv;

  NaClXMutexLock(&self->mu);
  while (self->num_clients >= self->max_clients) {
    NaClXCondVarWait(&self->cv, &self->mu);
  }
  NaClXMutexUnlock(&self->mu);

  /*
   * Other threads can only decrement num_clients.
   */
  rv = (*kNaClSimpleServiceVtbl.AcceptConnection)(vself, out);

  NaClXMutexLock(&self->mu);
  self->num_clients++;
  NaClXMutexUnlock(&self->mu);

  return rv;
}

void NaClSimpleLtdServiceRpcHandler(
    struct NaClSimpleService            *vself,
    struct NaClSimpleServiceConnection  *conn) {
  struct NaClSimpleLtdService *self =
      (struct NaClSimpleLtdService *) vself;

  (*kNaClSimpleServiceVtbl.RpcHandler)(vself, conn);
  NaClXMutexLock(&self->mu);
  self->num_clients--;
  NaClXCondVarBroadcast(&self->cv);
  NaClXMutexUnlock(&self->mu);
}


struct NaClSimpleServiceVtbl const kNaClSimpleLtdServiceVtbl = {
  {
    NaClSimpleLtdServiceDtor,
  },
  NaClSimpleServiceConnectionFactory,
  NaClSimpleLtdServiceAcceptConnection,
  NaClSimpleServiceAcceptAndSpawnHandler,
  NaClSimpleLtdServiceRpcHandler,
};
