/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SIMPLE_SERVICE_NACL_SIMPLE_SERVICE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SIMPLE_SERVICE_NACL_SIMPLE_SERVICE_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/nacl_compiler_annotations.h"

#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/trusted/nacl_base/nacl_refcount.h"
#include "native_client/src/trusted/threading/nacl_thread_interface.h"

EXTERN_C_BEGIN

struct NaClSimpleServiceConnection;  /* fwd */

struct NaClSimpleService {
  struct NaClRefCount               base NACL_IS_REFCOUNT_SUBCLASS;
  struct NaClDesc                   *bound_and_cap[2];
  /*
   * Ctor allocates bound socket and connection capability.  Dtor
   * closes.  Holds reference counted objects.  The connection
   * capability must be public, obviously; we don't provide an
   * accessor, so users can just reach inside and access directly.
   */

  struct NaClSrpcHandlerDesc const  *handlers;

  NaClThreadIfFactoryFunction       thread_factory_fn;
  void                              *thread_factory_data;

  struct NaClThreadInterface        *acceptor;
};

struct NaClSimpleServiceVtbl {
  struct NaClRefCountVtbl vbase;

  /*
   * A ConnectionFactory permits subclasses of
   * NaClSimpleServiceConnection objects to be used -- when the
   * NaClSimpleService need to create a NaClSimpleServiceConnection
   * object, the factory is asked to do so, and it can create any
   * subclass of NaClSimpleServiceConnection in its out parameter.
   * The return value of ConnectionFactory is kernel-ABI style, in
   * this case 0 for success and -NACL_ABI_E* for failure.  The *out
   * pointer does not point to live data on entry, and will point to a
   * fully constructed NaClSimpleServiceConnection object (or subclass
   * thereof) iff the connection_factory returns success.
   *
   * The ConnectionFactory is responsible for taking references to the
   * server (self) and conn parameters on successful return, if it
   * choses to keep the pointer values.  Since handlers is kept in the
   * NaClSimpleService object and the conn object must be live to
   * receive RPCs, generally taking additional references are
   * mandatory.  Generally, if the standard
   * NaClSimpleServiceConnectionCtor is used, then this is
   * automatically taken care of.
   *
   * The base class virtual function invokes the auxilliary function
   * NaClSimpleServiceConnectionFactoryWithInstanceData with self as
   * instance_data, which simply passes the instance_data argument to
   * the NaClSimleServiceConnectionCtor.  Subclasses that need more
   * complex per-connection instance data will probably wish to use
   * that auxilliary function directly.
   */
  int                     (*ConnectionFactory)(
      struct NaClSimpleService           *self,
      struct NaClDesc                    *conn,
      struct NaClSimpleServiceConnection **out);

  /*
   * Block invoking AcceptConn for a single connection request from a
   * client, then factory for a NaClSimpleServiceConnection object to
   * represent the connection to that client.
   */
  int                     (*AcceptConnection)(
      struct NaClSimpleService           *self,
      struct NaClSimpleServiceConnection **out);

  int                     (*AcceptAndSpawnHandler)(
      struct NaClSimpleService *self);

  /*
   * This method is invoked by the RPC handler thread and is expected
   * to loop handling RPCs until a break request or RPC channel
   * destruction occurs.  When this method returns, the |conn| object
   * is automatically Unref'd.  The Dtor of the |conn| object, should
   * it be invoked, is responsible for Unref'ing its reference to the
   * server object.
   *
   * Here's a use case.  Suppose we want to limit the number of
   * connections (and service threads) that a NaClSimpleService would
   * service at a time.  The AcceptConnection vfn would be overridden
   * to not accept connections (via the superclass AcceptConnection
   * vfn) until the connection counter is below a threshold; after
   * accepting a new connection, the counter is incremented.  The
   * RpcHandler vfn would be overriden in a subclass so that it
   * invokes the superclass Rpchandler, but on exit of the superclass
   * RpcHandler, decrements the connection counter.
   */
  void                    (*RpcHandler)(
      struct NaClSimpleService            *self,
      struct NaClSimpleServiceConnection  *conn);
};

struct NaClSimpleServiceConnection {
  struct NaClRefCount         base NACL_IS_REFCOUNT_SUBCLASS;

  /* holds refs */
  struct NaClSimpleService    *server;
  struct NaClDesc             *connected_socket;

  void                        *instance_data;

  struct NaClThreadInterface  *thread;
  /* other data is application specific, in subclasses only */
};

int NaClSimpleServiceConnectionCtor(
    struct NaClSimpleServiceConnection  *self,
    struct NaClSimpleService            *server,
    struct NaClDesc                     *conn,
    void                                *instance_data);

/*
 * This Dtor should only be called after the thread has exited.
 */
void NaClSimpleServiceConnectionDtor(struct NaClRefCount *vself);

int NaClSimpleServiceConnectionServerLoop(
    struct NaClSimpleServiceConnection *self);

int NaClSimpleServiceCtor(
    struct NaClSimpleService          *self,
    struct NaClSrpcHandlerDesc const  *srpc_handlers,
    NaClThreadIfFactoryFunction       thread_factory_fn,
    void                              *thread_factory_data);

int NaClSimpleServiceWithSocketCtor(
    struct NaClSimpleService          *self,
    struct NaClSrpcHandlerDesc const  *srpc_handlers,
    NaClThreadIfFactoryFunction       thread_factory_fn,
    void                              *thread_factory_data,
    struct NaClDesc                   *service_port,
    struct NaClDesc                   *sock_addr);

void NaClSimpleServiceDtor(struct NaClRefCount *vself);

/*
 * Will take a new reference to instance data to pass to the
 * Connection Ctor.
 */
int NaClSimpleServiceConnectionFactoryWithInstanceData(
    struct NaClSimpleService            *self,
    struct NaClDesc                     *conn,
    void                                *instance_data,
    struct NaClSimpleServiceConnection  **out);

int NaClSimpleServiceConnectionFactory(
    struct NaClSimpleService            *self,
    struct NaClDesc                     *conn,
    struct NaClSimpleServiceConnection  **out);

int NaClSimpleServiceAcceptConnection(
    struct NaClSimpleService            *self,
    struct NaClSimpleServiceConnection  **out);

int NaClSimpleServiceAcceptAndSpawnHandler(
    struct NaClSimpleService *self);

void NaClSimpleServiceRpcHandler(
    struct NaClSimpleService            *self,
    struct NaClSimpleServiceConnection  *conn);

struct NaClSimpleServiceConnectionVtbl {
  struct NaClRefCountVtbl vbase;

  /*
   * Takes ownership of the reference.  Invokes NaClSrpcServerLoop.
   */
  int (*ServerLoop)(struct NaClSimpleServiceConnection *self);
};

extern struct NaClSimpleServiceVtbl const kNaClSimpleServiceVtbl;
extern struct NaClSimpleServiceConnectionVtbl const
  kNaClSimpleServiceConnectionVtbl;


int NaClSimpleServiceAcceptAndSpawnHandler(
    struct NaClSimpleService *server);

int NaClSimpleServiceStartServiceThread(struct NaClSimpleService *server);

EXTERN_C_END

#endif
