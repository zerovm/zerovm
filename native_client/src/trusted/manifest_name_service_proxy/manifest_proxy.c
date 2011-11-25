/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/trusted/manifest_name_service_proxy/manifest_proxy.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/shared/srpc/nacl_srpc.h"
#include "native_client/src/trusted/reverse_service/manifest_rpc.h"
#include "native_client/src/trusted/reverse_service/reverse_control_rpc.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/nacl_name_service.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

static void NaClManifestWaitForChannel_yield_mu(
    struct NaClManifestProxyConnection *self) {
  NaClLog(4, "Entered NaClManifestWaitForChannel_yield_mu\n");
  NaClXMutexLock(&self->mu);
  NaClLog(4, "NaClManifestWaitForChannel_yield_mu: checking channel\n");
  while (!self->channel_initialized) {
    NaClLog(4, "NaClManifestWaitForChannel_yield_mu: waiting\n");
    NaClXCondVarWait(&self->cv, &self->mu);
  }
  NaClLog(4, "Leaving NaClManifestWaitForChannel_yield_mu\n");
}

static void NaClManifestReleaseChannel_release_mu(
    struct NaClManifestProxyConnection *self) {
  NaClLog(4, "NaClManifestReleaseChannel_release_mu\n");
  NaClXMutexUnlock(&self->mu);
}

static void NaClManifestNameServiceListRpc(
    struct NaClSrpcRpc      *rpc,
    struct NaClSrpcArg      **in_args,
    struct NaClSrpcArg      **out_args,
    struct NaClSrpcClosure  *done_cls) {
  struct NaClManifestProxyConnection  *proxy_conn =
      (struct NaClManifestProxyConnection *) rpc->channel->server_instance_data;
  uint32_t                            nbytes = out_args[0]->u.count;
  char                                *dest = out_args[0]->arrays.carr;
  NaClSrpcError                       srpc_error;

  UNREFERENCED_PARAMETER(in_args);
  NaClLog(4,
          "NaClManifestNameServiceListRpc, proxy_conn 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) proxy_conn);

  NaClManifestWaitForChannel_yield_mu(proxy_conn);

  NaClLog(4,
          ("NaClManifestNameServiceListRpc: nbytes %"NACL_PRIu32", dest"
           " 0x%"NACL_PRIxPTR"\n"),
          nbytes, (uintptr_t) dest);
  if (NACL_SRPC_RESULT_OK !=
      (srpc_error =
       NaClSrpcInvokeBySignature(&proxy_conn->client_channel,
                                 NACL_MANIFEST_LIST,
                                 &nbytes, dest))) {
    NaClLog(LOG_ERROR,
            ("Manifest list via channel 0x%"NACL_PRIxPTR" with RPC "
             NACL_MANIFEST_LIST" failed: %d\n"),
            (uintptr_t) &proxy_conn->client_channel,
            srpc_error);
    rpc->result = srpc_error;
  } else {
    NaClLog(3,
            "NaClManifestNameServiceListRpc, proxy returned %"NACL_PRId32
            " bytes\n",
            nbytes);
    out_args[0]->u.count = nbytes;
    rpc->result = NACL_SRPC_RESULT_OK;
  }
  (*done_cls->Run)(done_cls);
  NaClManifestReleaseChannel_release_mu(proxy_conn);
}

static void NaClManifestNameServiceInsertRpc(
    struct NaClSrpcRpc      *rpc,
    struct NaClSrpcArg      **in_args,
    struct NaClSrpcArg      **out_args,
    struct NaClSrpcClosure  *done_cls) {

  UNREFERENCED_PARAMETER(in_args);
  NaClLog(4, "NaClManifestNameServiceInsertRpc\n");
  out_args[0]->u.ival = NACL_NAME_SERVICE_PERMISSION_DENIED;
  /* cannot add names to the manifest! */
  rpc->result = NACL_SRPC_RESULT_OK;
  (*done_cls->Run)(done_cls);
}

static void NaClManifestNameServiceLookupRpc(
    struct NaClSrpcRpc      *rpc,
    struct NaClSrpcArg      **in_args,
    struct NaClSrpcArg      **out_args,
    struct NaClSrpcClosure  *done_cls) {
  struct NaClManifestProxyConnection  *proxy_conn =
      (struct NaClManifestProxyConnection *) rpc->channel->server_instance_data;
  char                                *name = in_args[0]->arrays.str;
  int                                 flags = in_args[1]->u.ival;
  char                                cookie[20];
  uint32_t                            cookie_size = sizeof cookie;
  int                                 status;
  struct NaClDesc                     *desc;
  NaClSrpcError                       srpc_error;

  NaClLog(4, "NaClManifestNameServiceLookupRpc\n");

  NaClManifestWaitForChannel_yield_mu(proxy_conn);

  NaClLog(4,
          "NaClManifestNameServiceLookupRpc: name %s, flags %d\n",
          name, flags);
  NaClLog(4,
          "NaClManifestNameServiceLookupRpc: invoking %s\n",
          NACL_MANIFEST_LOOKUP);

  if (NACL_SRPC_RESULT_OK !=
      (srpc_error =
       NaClSrpcInvokeBySignature(&proxy_conn->client_channel,
                                 NACL_MANIFEST_LOOKUP,
                                 name,
                                 flags,
                                 &status,
                                 &desc,
                                 &cookie_size,
                                 cookie))) {
    NaClLog(LOG_ERROR,
            ("Manifest lookup via channel 0x%"NACL_PRIxPTR" with RPC "
             NACL_MANIFEST_LOOKUP" failed: %d\n"),
            (uintptr_t) &proxy_conn->client_channel,
            srpc_error);
    rpc->result = srpc_error;
  } else {
    NaClLog(4,
            "NaClManifestNameServiceLookupRpc: got cookie %.*s\n",
            cookie_size, cookie);
    out_args[0]->u.ival = status;
    out_args[1]->u.hval = desc;
    rpc->result = NACL_SRPC_RESULT_OK;
  }
  (*done_cls->Run)(done_cls);
  NaClManifestReleaseChannel_release_mu(proxy_conn);
}

static void NaClManifestNameServiceDeleteRpc(
    struct NaClSrpcRpc      *rpc,
    struct NaClSrpcArg      **in_args,
    struct NaClSrpcArg      **out_args,
    struct NaClSrpcClosure  *done_cls) {

  UNREFERENCED_PARAMETER(in_args);
  NaClLog(4, "NaClManifestNameServiceDeleteRpc\n");
  out_args[0]->u.ival = NACL_NAME_SERVICE_PERMISSION_DENIED;
  rpc->result = NACL_SRPC_RESULT_OK;
  (*done_cls->Run)(done_cls);
}

struct NaClSrpcHandlerDesc const kNaClManifestProxyHandlers[] = {
  { NACL_NAME_SERVICE_LIST, NaClManifestNameServiceListRpc, },
  { NACL_NAME_SERVICE_INSERT, NaClManifestNameServiceInsertRpc, },
  { NACL_NAME_SERVICE_LOOKUP, NaClManifestNameServiceLookupRpc, },
  { NACL_NAME_SERVICE_DELETE, NaClManifestNameServiceDeleteRpc, },
  { (char const *) NULL, (NaClSrpcMethod) NULL, },
};


int NaClManifestProxyCtor(struct NaClManifestProxy    *self,
                          NaClThreadIfFactoryFunction thread_factory_fn,
                          void                        *thread_factory_data,
                          struct NaClApp              *nap) {
  NaClLog(4,
          ("Entered NaClManifestProxyCtor: self 0x%"NACL_PRIxPTR
           ", nap 0x%"NACL_PRIxPTR"\n"),
          (uintptr_t) self,
          (uintptr_t) nap);
  if (!NaClSimpleServiceCtor(&self->base,
                             kNaClManifestProxyHandlers,
                             thread_factory_fn,
                             thread_factory_data)) {
    return 0;
  }
  self->nap = nap;
  NACL_VTBL(NaClRefCount, self) =
      (struct NaClRefCountVtbl *) &kNaClManifestProxyVtbl;
  return 1;
}

static void NaClManifestProxyDtor(struct NaClRefCount *vself) {
  struct NaClManifestProxy *self =
      (struct NaClManifestProxy *) vself;

  NACL_VTBL(NaClRefCount, self) =
      (struct NaClRefCountVtbl *) &kNaClSimpleServiceVtbl;
  (*NACL_VTBL(NaClRefCount, self)->Dtor)(vself);
}

int NaClManifestProxyConnectionCtor(struct NaClManifestProxyConnection  *self,
                                    struct NaClManifestProxy            *server,
                                    struct NaClDesc                     *conn) {
  NaClLog(4,
          "Entered NaClManifestProxyConnectionCtor, self 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) self);
  if (!NaClSimpleServiceConnectionCtor(
          &self->base,
          (struct NaClSimpleService *) server,
          conn,
          (void *) self)) {
    NaClLog(4,
            ("NaClManifestProxyConnectionCtor: base class ctor"
             " NaClSimpleServiceConnectionCtor failed\n"));
    return 0;
  }
  NaClXMutexCtor(&self->mu);
  NaClXCondVarCtor(&self->cv);
  self->channel_initialized = 0;
  NACL_VTBL(NaClRefCount, self) =
      (struct NaClRefCountVtbl *) &kNaClManifestProxyConnectionVtbl;
  return 1;
}

void NaClManifestProxyConnectionRevHandleConnect(
    struct NaClManifestProxyConnection  *self,
    struct NaClDesc                     *rev) {
  NaClLog(4, "Entered NaClManifestProxyConnectionRevHandleConnect\n");
  NaClXMutexLock(&self->mu);
  if (self->channel_initialized) {
    NaClLog(LOG_FATAL,
            "NaClManifestProxyConnectionRevHandleConnect: double connect?\n");
  }
  /*
   * If NaClSrpcClientCtor proves to take too long, we should spin off
   * another thread to do the initialization so that the reverse
   * client can accept additional reverse channels.
   */
  NaClLog(4,
          "NaClManifestProxyConnectionRevHandleConnect: Creating SrpcClient\n");
  if (NaClSrpcClientCtor(&self->client_channel, rev)) {
    NaClLog(4,
            ("NaClManifestProxyConnectionRevHandleConnect: SrpcClientCtor"
             " succeded, announcing.\n"));
    self->channel_initialized = 1;
    NaClXCondVarBroadcast(&self->cv);
    /* ownership of rev taken */
  } else {
    NaClLog(4,
            ("NaClManifestProxyConnectionRevHandleConnect: NaClSrpcClientCtor"
             " failed\n"));
  }
  NaClXMutexUnlock(&self->mu);
  NaClLog(4, "Leaving NaClManifestProxyConnectionRevHandleConnect\n");
}

static void NaClManifestProxyConnectionDtor(struct NaClRefCount *vself) {
  struct NaClManifestProxyConnection *self =
      (struct NaClManifestProxyConnection *) vself;
  NaClLog(4,
          "Entered NaClManifestProxyConnectionDtor: self 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) self);
  NaClXMutexLock(&self->mu);
  while (!self->channel_initialized) {
    NaClLog(4,
            "NaClManifestProxyConnectionDtor:"
            " waiting for connection initialization\n");
    NaClXCondVarWait(&self->cv, &self->mu);
  }
  NaClXMutexUnlock(&self->mu);

  NaClLog(4, "NaClManifestProxyConnectionDtor: dtoring\n");

  NaClCondVarDtor(&self->cv);
  NaClMutexDtor(&self->mu);

  NaClSrpcDtor(&self->client_channel);
  NACL_VTBL(NaClSimpleServiceConnection, self) =
      &kNaClSimpleServiceConnectionVtbl;
  (*NACL_VTBL(NaClRefCount, self)->Dtor)(vself);
}

/*
 * NaClManifestProxyConnection is a NaClSimpleServiceConnection
 */
struct NaClSimpleServiceConnectionVtbl
  const kNaClManifestProxyConnectionVtbl = {
  {
    NaClManifestProxyConnectionDtor,
  },
  NaClSimpleServiceConnectionServerLoop,
};

static void NaClManifestReverseClientCallback(
    void                        *state,
    struct NaClThreadInterface  *tif,
    struct NaClDesc             *new_conn) {
  struct NaClManifestProxyConnection *mconn =
      (struct NaClManifestProxyConnection *) state;

  UNREFERENCED_PARAMETER(tif);
  NaClLog(4, "Entered NaClManifestReverseClientCallback\n");
  NaClManifestProxyConnectionRevHandleConnect(mconn, new_conn);
}

int NaClManifestProxyConnectionFactory(
    struct NaClSimpleService            *vself,
    struct NaClDesc                     *conn,
    struct NaClSimpleServiceConnection  **out) {
  struct NaClManifestProxy            *self =
      (struct NaClManifestProxy *) vself;
  struct NaClApp                      *nap = self->nap;
  struct NaClManifestProxyConnection  *mconn;
  NaClSrpcError                       rpc_result;
  int                                 bool_status;

  NaClLog(4,
          ("Entered NaClManifestProxyConnectionFactory, self 0x%"NACL_PRIxPTR
           "\n"),
          (uintptr_t) self);
  mconn = (struct NaClManifestProxyConnection *) malloc(sizeof *mconn);
  if (NULL == mconn) {
    NaClLog(4, "NaClManifestProxyConnectionFactory: no memory\n");
    return -NACL_ABI_ENOMEM;
  }
  NaClLog(4, "NaClManifestProxyConnectionFactory: creating connection obj\n");
  if (!NaClManifestProxyConnectionCtor(mconn, self, conn)) {
    free(mconn);
    return -NACL_ABI_EIO;
  }

  /*
   * Construct via NaClSecureReverseClientCtor with a callback to
   * process the new reverse connection -- which should be stored in
   * the mconn object.
   *
   * Make reverse RPC to obtain a new reverse RPC connection.
   */
  NaClLog(4, "NaClManifestProxyConnectionFactory: locking reverse channel\n");
  NaClLog(4, "NaClManifestProxyConnectionFactory: nap 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) nap);
  NaClXMutexLock(&nap->mu);
  if (NACL_REVERSE_CHANNEL_INITIALIZED !=
      nap->reverse_channel_initialization_state) {
    NaClLog(LOG_FATAL,
            "NaClManifestProxyConnectionFactory invoked w/o reverse channel\n");
  }
  NaClLog(4, "NaClManifestProxyConnectionFactory: inserting handler\n");
  if (!NaClSecureReverseClientInsertHandler(nap->reverse_client,
                                            NaClManifestReverseClientCallback,
                                            (void *) mconn)) {
    NaClLog(LOG_FATAL,
            ("NaClManifestProxyConnectionFactory:"
             " NaClSecureReverseClientInsertHandler failed\n"));
  }
  /*
   * NaClSrpcInvokeBySignature(""); tell plugin to connect and create
   * a reverse channel
   */
  NaClLog(4,
          ("NaClManifestProxyConnectionFactory: making RPC"
           " to set up connection\n"));
  rpc_result = NaClSrpcInvokeBySignature(&nap->reverse_channel,
                                         NACL_REVERSE_CONTROL_ADD_CHANNEL,
                                         &bool_status);
  if (NACL_SRPC_RESULT_OK != rpc_result) {
    NaClLog(LOG_FATAL,
            "NaClManifestProxyConnectionFactory: add channel RPC failed: %d",
            rpc_result);
  }
  NaClLog(4,
          "NaClManifestProxyConnectionFactory: Start status %d\n", bool_status);

  NaClXMutexUnlock(&nap->mu);

  *out = (struct NaClSimpleServiceConnection *) mconn;
  return 0;
}

struct NaClSimpleServiceVtbl const kNaClManifestProxyVtbl = {
  {
    NaClManifestProxyDtor,
  },
  NaClManifestProxyConnectionFactory,
  /* see name_service.c vtbl for connection factory and ownership */
  /*
   * The NaClManifestProxyConnectionFactory creates a subclass of a
   * NaClSimpleServiceConnectionFactory object that uses the reverse
   * connection object nap->reverse_client to obtain a new RPC channel
   * with each manifest connection.
   */
  NaClSimpleServiceAcceptConnection,
  NaClSimpleServiceAcceptAndSpawnHandler,
  NaClSimpleServiceRpcHandler,
};
