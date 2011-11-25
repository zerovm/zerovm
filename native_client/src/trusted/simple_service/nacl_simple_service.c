/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/simple_service/nacl_simple_service.h"

#include "native_client/src/include/nacl_compiler_annotations.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/shared/srpc/nacl_srpc.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_conn_cap.h"
#include "native_client/src/trusted/desc/nrd_xfer.h"

#include "native_client/src/trusted/nacl_base/nacl_refcount.h"

#include "native_client/src/trusted/service_runtime/nacl_config.h"
/* NACL_KERN_STACK_SIZE */
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"

#include "native_client/src/trusted/threading/nacl_thread_interface.h"

int NaClSimpleServiceConnectionCtor(
    struct NaClSimpleServiceConnection  *self,
    struct NaClSimpleService            *server,
    struct NaClDesc                     *conn,
    void                                *instance_data) {
  NaClLog(4,
          "NaClSimpleServiceConnectionCtor: self 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) self);
  if (!NaClRefCountCtor((struct NaClRefCount *) self)) {
    return 0;
  }
  self->server = (struct NaClSimpleService *) NaClRefCountRef(
      (struct NaClRefCount *) server);
  self->connected_socket = (struct NaClDesc *) NaClRefCountRef(
      (struct NaClRefCount *) conn);
  self->instance_data = instance_data;

  self->thread = NULL;

  self->base.vtbl = (struct NaClRefCountVtbl const *)
      &kNaClSimpleServiceConnectionVtbl;
  return 1;
}

void NaClSimpleServiceConnectionDtor(struct NaClRefCount *vself) {
  struct NaClSimpleServiceConnection *self =
      (struct NaClSimpleServiceConnection *) vself;

  NaClRefCountUnref((struct NaClRefCount *) self->server);
  NaClRefCountUnref((struct NaClRefCount *) self->connected_socket);

  NACL_VTBL(NaClRefCount, self) = &kNaClRefCountVtbl;
  (*NACL_VTBL(NaClRefCount, self)->Dtor)(vself);
}

int NaClSimpleServiceConnectionServerLoop(
    struct NaClSimpleServiceConnection *self) {
  int retval;

  NaClLog(4,
          "NaClSimpleServiceConnectionServerLoop: handling RPC requests from"
          " client at socket %"NACL_PRIxPTR"\n",
          (uintptr_t) self->connected_socket);
  retval = NaClSrpcServerLoop(self->connected_socket,
                              self->server->handlers,
                              self->instance_data);
  NaClLog(4,
          "NaClSimpleServiceConnectionServerLoop: NaClSrpcServerLoop exited,"
          " value %d\n", retval);
  return retval;
}

struct NaClSimpleServiceConnectionVtbl
const kNaClSimpleServiceConnectionVtbl = {
  {
    NaClSimpleServiceConnectionDtor,
  },
  NaClSimpleServiceConnectionServerLoop,
};


/* default rpc_handler */
void NaClSimpleServiceRpcHandler(
    struct NaClSimpleService           *self,
    struct NaClSimpleServiceConnection *conn) {
  UNREFERENCED_PARAMETER(self);

  (*NACL_VTBL(NaClSimpleServiceConnection, conn)->ServerLoop)(
      conn);
}

static void *RpcHandlerBase(struct NaClThreadInterface *tif) {
  struct NaClSimpleServiceConnection  *conn =
      (struct NaClSimpleServiceConnection *) tif->thread_data;

  NaClLog(4, "Entered RpcHandlerBase, invoking RpcHandler virtual fn\n");
  (*NACL_VTBL(NaClSimpleService, conn->server)->RpcHandler)(conn->server,
                                                            conn);
  NaClLog(4, "Leaving RpcHandlerBase\n");
  /*
   * The unref of conn may Dtor the currently running thread.  This is
   * okay, since this only removes the ability to use the thread
   * handle (in Windows) but does not otherwise affect the thread.  We
   * don't log afterwards, just in case the logging code (is later
   * modified to) use thread info.
   */
  NaClRefCountUnref((struct NaClRefCount *) conn);
  return (void *) NULL;
}

static int NaClSimpleServiceCtorIntern(
    struct NaClSimpleService          *self,
    struct NaClSrpcHandlerDesc const  *srpc_handlers,
    NaClThreadIfFactoryFunction       thread_factory_fn,
    void                              *thread_factory_data) {
  NaClLog(4,
          "NaClSimpleServiceCtorIntern, self 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) self);
  if (!NaClRefCountCtor((struct NaClRefCount *) self)) {
    NaClLog(4, "NaClSimpleServiceCtorIntern: NaClRefCountCtor failed\n");
    return 0;
  }
  self->handlers = srpc_handlers;
  self->thread_factory_fn = thread_factory_fn;
  self->thread_factory_data = thread_factory_data;
  self->acceptor = (struct NaClThreadInterface *) NULL;

  self->base.vtbl = (struct NaClRefCountVtbl const *) &kNaClSimpleServiceVtbl;
  NaClLog(4, "Leaving NaClSimpleServiceCtorIntern\n");
  return 1;
}

int NaClSimpleServiceCtor(
    struct NaClSimpleService          *self,
    struct NaClSrpcHandlerDesc const  *srpc_handlers,
    NaClThreadIfFactoryFunction       thread_factory_fn,
    void                              *thread_factory_data) {
  NaClLog(4,
          "Entered NaClSimpleServiceCtor: self 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) self);

  if (0 != NaClCommonDescMakeBoundSock(self->bound_and_cap)) {
    return 0;
  }
  if (!NaClSimpleServiceCtorIntern(self, srpc_handlers,
                                   thread_factory_fn, thread_factory_data)) {
    NaClDescUnref(self->bound_and_cap[0]);
    NaClDescUnref(self->bound_and_cap[1]);
    return 0;
  }
  return 1;
}

int NaClSimpleServiceWithSocketCtor(
    struct NaClSimpleService          *self,
    struct NaClSrpcHandlerDesc const  *srpc_handlers,
    NaClThreadIfFactoryFunction       thread_factory_fn,
    void                              *thread_factory_data,
    struct NaClDesc                   *service_port,
    struct NaClDesc                   *sock_addr) {
  NaClLog(4,
          "NaClSimpleServiceWithSocketCtor: self 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) self);
  if (!NaClSimpleServiceCtorIntern(self, srpc_handlers,
                                   thread_factory_fn, thread_factory_data)) {
    return 0;
  }
  self->bound_and_cap[0] = NaClDescRef(service_port);
  self->bound_and_cap[1] = NaClDescRef(sock_addr);

  return 1;
}

void NaClSimpleServiceDtor(struct NaClRefCount *vself) {
  struct NaClSimpleService *self = (struct NaClSimpleService *) vself;

  NaClRefCountSafeUnref((struct NaClRefCount *) self->bound_and_cap[0]);
  NaClRefCountSafeUnref((struct NaClRefCount *) self->bound_and_cap[1]);

  NACL_VTBL(NaClRefCount, self) = &kNaClRefCountVtbl;
  (*NACL_VTBL(NaClRefCount, self)->Dtor)(vself);
}

int NaClSimpleServiceConnectionFactoryWithInstanceData(
    struct NaClSimpleService            *self,
    struct NaClDesc                     *conn,
    void                                *instance_data,
    struct NaClSimpleServiceConnection  **out) {
  struct NaClSimpleServiceConnection  *server_conn;
  int                                 status;

  NaClLog(4, "Entered NaClSimpleServiceConnectionFactory\n");
  server_conn = malloc(sizeof *server_conn);
  if (NULL == server_conn) {
    status = -NACL_ABI_ENOMEM;
    goto abort;
  }
  NaClLog(4,
          "NaClSimpleServiceConnectionFactory: self 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) self);
  NaClLog(4,
          "NaClSimpleServiceConnectionFactory: conn 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) conn);
  NaClLog(4,
          "NaClSimpleServiceConnectionFactory: out 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) out);
  /*
   * In the common/degenerate case, factory instance data becomes
   * connection instance data.
   */
  if (!NaClSimpleServiceConnectionCtor(server_conn,
                                       self,
                                       conn,
                                       instance_data)) {
    free(server_conn);
    status = -NACL_ABI_EIO;
    goto abort;
  }
  NaClLog(4,
          "NaClSimpleServiceConnectionFactory: constructed 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) server_conn);
  *out = server_conn;
  status = 0;
 abort:
  NaClLog(4, "Leaving NaClSimpleServiceConnectionFactory: status %d\n", status);
  return status;
}

int NaClSimpleServiceConnectionFactory(
    struct NaClSimpleService            *self,
    struct NaClDesc                     *conn,
    struct NaClSimpleServiceConnection  **out) {
  return NaClSimpleServiceConnectionFactoryWithInstanceData(
      self, conn, (struct NaClRefCount *) self, out);
}

int NaClSimpleServiceAcceptConnection(
    struct NaClSimpleService            *self,
    struct NaClSimpleServiceConnection  **out) {
  int                                 status = -NACL_ABI_EINVAL;
  struct NaClSimpleServiceConnection  *conn = NULL;
  struct NaClDesc                     *connected_desc = NULL;

  NaClLog(4, "Entered NaClSimpleServiceAcceptConnection\n");
  conn = malloc(sizeof *conn);
  if (NULL == conn) {
    return -NACL_ABI_ENOMEM;
  }
  /* NB: conn is allocated but not constructed */
  status = (*NACL_VTBL(NaClDesc, self->bound_and_cap[0])->
            AcceptConn)(self->bound_and_cap[0], &connected_desc);
  if (0 != status) {
    NaClLog(4, "Accept failed\n");
    free(conn);
    conn = NULL;
    goto cleanup;
  }

  NaClLog(4,
          "connected_desc is 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) connected_desc);

  status = (*NACL_VTBL(NaClSimpleService, self)->ConnectionFactory)(
      self,
      connected_desc,
      &conn);
  if (0 != status) {
    NaClLog(4, "ConnectionFactory failed\n");
    goto cleanup;
  }
  /* all okay! */

  NaClLog(4,
          "NaClSimpleServiceAcceptConnection: conn is 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) conn);
  NaClLog(4,
          "NaClSimpleServiceAcceptConnection: out  is 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) out);
  *out = conn;
  status = 0;
cleanup:
  NaClRefCountSafeUnref((struct NaClRefCount *) connected_desc);
  NaClLog(4, "Leaving NaClSimpleServiceAcceptConnection, status %d\n", status);
  return status;
}

int NaClSimpleServiceAcceptAndSpawnHandler(
    struct NaClSimpleService *self) {
  struct NaClSimpleServiceConnection  *conn = NULL;
  int                                 status;

  NaClLog(4, "Entered NaClSimpleServiceAcceptAndSpawnHandler\n");
  NaClLog(4,
          ("NaClSimpleServiceAcceptAndSpawnHandler:"
           " &conn is 0x%"NACL_PRIxPTR"\n"),
          (uintptr_t) &conn);
  status = (*NACL_VTBL(NaClSimpleService, self)->AcceptConnection)(
      self,
      &conn);
  NaClLog(4, "AcceptConnection vfn returned %d\n", status);
  if (0 != status) {
    goto abort;
  }
  NaClLog(4,
          "NaClSimpleServiceAcceptAndSpawnHandler: conn is 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) conn);
  NaClLog(4, "NaClSimpleServiceAcceptAndSpawnHandler: spawning thread\n");

  CHECK(NULL == conn->thread);

  /* ownership of |conn| reference is passed to the thread */
  if (!NaClThreadInterfaceConstructAndStartThread(
          self->thread_factory_fn,
          self->thread_factory_data,
          RpcHandlerBase,
          conn,
          NACL_KERN_STACK_SIZE,
          &conn->thread)) {
    NaClLog(4, "NaClSimpleServiceAcceptAndSpawnHandler: no thread\n");
    NaClRefCountUnref((struct NaClRefCount *) conn);
    conn = NULL;
    conn->thread = NULL;
    status = -NACL_ABI_EAGAIN;
    goto abort;
  }
  status = 0;
abort:
  NaClLog(4,
          "Leaving NaClSimpleServiceAcceptAndSpawnHandler, status %d\n",
          status);
  return status;
}

struct NaClSimpleServiceVtbl const kNaClSimpleServiceVtbl = {
  {
    NaClSimpleServiceDtor,
  },
  NaClSimpleServiceConnectionFactory,
  NaClSimpleServiceAcceptConnection,
  NaClSimpleServiceAcceptAndSpawnHandler,
  NaClSimpleServiceRpcHandler,
};


static void *AcceptThread(struct NaClThreadInterface *tif) {
  struct NaClSimpleService *server =
      (struct NaClSimpleService *) tif->thread_data;

  NaClLog(4, "Entered AcceptThread\n");
  while (0 == (*NACL_VTBL(NaClSimpleService, server)->
               AcceptAndSpawnHandler)(server)) {
    NaClLog(4, "AcceptThread: accepted, looping to next thread\n");
    continue;
  }
  NaClRefCountUnref(&server->base);
  return (void *) NULL;
}

int NaClSimpleServiceStartServiceThread(struct NaClSimpleService *server) {
  NaClLog(4, "NaClSimpleServiceStartServiceThread: spawning thread\n");
  CHECK(server->acceptor == NULL);

  if (!NaClThreadInterfaceConstructAndStartThread(
          server->thread_factory_fn,
          server->thread_factory_data,
          AcceptThread,
          NaClRefCountRef(&server->base),
          NACL_KERN_STACK_SIZE,
          &server->acceptor)) {
    NaClLog(4, "NaClSimpleServiceStartServiceThread: no thread\n");
    NaClRefCountUnref(&server->base);  /* undo ref in Ctor call arglist */
    server->acceptor = NULL;
    return 0;
  }
  NaClLog(4, "NaClSimpleServiceStartServiceThread: success\n");
  return 1;
}
