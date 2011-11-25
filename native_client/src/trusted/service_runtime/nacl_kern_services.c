/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * A simple service for "kernel services".  The socket address will be
 * available to the NaCl module.
 */

#include "native_client/src/trusted/service_runtime/include/sys/nacl_kern_rpc.h"
#include "native_client/src/trusted/service_runtime/nacl_kern_services.h"

#include "native_client/src/trusted/reverse_service/reverse_control_rpc.h"
#include "native_client/src/trusted/simple_service/nacl_simple_service.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/shared/srpc/nacl_srpc.h"

static void NaClKernServiceInitializationCompleteRpc(
    struct NaClSrpcRpc      *rpc,
    struct NaClSrpcArg      **in_args,
    struct NaClSrpcArg      **out_args,
    struct NaClSrpcClosure  *done_cls) {
  struct NaClKernService  *service =
      (struct NaClKernService *) rpc->channel->server_instance_data;
  struct NaClApp          *nap;
  NaClSrpcError           rpc_result;

  UNREFERENCED_PARAMETER(in_args);
  UNREFERENCED_PARAMETER(out_args);

  rpc->result = NACL_SRPC_RESULT_OK;
  (*done_cls->Run)(done_cls);

  NaClLog(4,
          "NaClKernServiceInitializationCompleteRpc: nap 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) service->nap);
  nap = service->nap;
  NaClXMutexLock(&nap->mu);
  if (NACL_REVERSE_CHANNEL_INITIALIZED ==
      nap->reverse_channel_initialization_state) {
    rpc_result = NaClSrpcInvokeBySignature(&nap->reverse_channel,
                                           NACL_REVERSE_CONTROL_INIT_DONE);
    if (NACL_SRPC_RESULT_OK != rpc_result) {
      NaClLog(LOG_FATAL,
              "NaClKernService: InitDone RPC failed: %d\n", rpc_result);
    }
  } else {
    NaClLog(3, "NaClKernService: no reverse channel, no plugin to talk to.\n");
  }
  NaClXMutexUnlock(&nap->mu);
}

struct NaClSrpcHandlerDesc const kNaClKernServiceHandlers[] = {
  { NACL_KERN_SERVICE_INITIALIZATION_COMPLETE,
    NaClKernServiceInitializationCompleteRpc, },
  { (char const *) NULL, (NaClSrpcMethod) NULL, },
};

int NaClKernServiceCtor(
    struct NaClKernService      *self,
    NaClThreadIfFactoryFunction thread_factory_fn,
    void                        *thread_factory_data,
    struct NaClApp              *nap) {
  NaClLog(4,
          ("NaClKernServiceCtor: self 0x%"NACL_PRIxPTR", nap 0x%"
           NACL_PRIxPTR"\n"),
          (uintptr_t) self,
          (uintptr_t) nap);
  if (!NaClSimpleServiceCtor(&self->base,
                             kNaClKernServiceHandlers,
                             thread_factory_fn,
                             thread_factory_data)) {
    return 0;
  }
  self->nap = nap;
  NACL_VTBL(NaClRefCount, self) =
      (struct NaClRefCountVtbl *) &kNaClKernServiceVtbl;
  return 1;
}

void NaClKernServiceDtor(struct NaClRefCount *vself) {
  struct NaClKernService *self = (struct NaClKernService *) vself;
  NACL_VTBL(NaClRefCount, self) =
      (struct NaClRefCountVtbl *) &kNaClSimpleServiceVtbl;
  (*NACL_VTBL(NaClRefCount, self)->Dtor)(vself);
}

struct NaClSimpleServiceVtbl const kNaClKernServiceVtbl = {
  {
    NaClKernServiceDtor,
  },
  NaClSimpleServiceConnectionFactory,
  NaClSimpleServiceAcceptConnection,
  NaClSimpleServiceAcceptAndSpawnHandler,
  NaClSimpleServiceRpcHandler,
};
