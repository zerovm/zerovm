/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_MANIFEST_NAME_SERVICE_PROXY_MANIFEST_PROXY_H_
#define NATIVE_CLIENT_SRC_TRUSTED_MANIFEST_NAME_SERVICE_PROXY_MANIFEST_PROXY_H_

#include "native_client/src/include/nacl_base.h"

#include "native_client/src/shared/srpc/nacl_srpc.h"
#include "native_client/src/trusted/service_runtime/sel_ldr_thread_interface.h"
#include "native_client/src/trusted/simple_service/nacl_simple_service.h"

EXTERN_C_BEGIN

/*
 * Trusted SRPC server that proxies name service lookups to the
 * browser plugin.  This is Pepper2-specific code that could, in
 * principle, be generalized.
 *
 * Manifest names are "short names".  These are often sonames for
 * dynamic libraries, but may be any read-only resource that the NaCl
 * application needs, e.g., level data files, audio samples, etc.
 */

struct NaClManifestProxy {
  struct NaClSimpleService  base NACL_IS_REFCOUNT_SUBCLASS;

  struct NaClApp  *nap;
};

struct NaClManifestProxyConnection {
  struct NaClSimpleServiceConnection  base NACL_IS_REFCOUNT_SUBCLASS;

  struct NaClMutex                    mu;
  struct NaClCondVar                  cv;
  int                                 channel_initialized;
  struct NaClSrpcChannel              client_channel;
};

extern struct NaClSimpleServiceVtbl const kNaClManifestProxyVtbl;

/*
 * The nap reference is used by the connection factory to initiate a
 * reverse connection: the connection factory enqueue a callback via
 * the NaClSecureReverseClientCtor that wakes up the connection
 * factory which issues an upcall on the existing reverse connection
 * to ask for a new one.  When the new connection arrives, the
 * NaClManifestProxyConnectionFactory can wrap the reverse channel in
 * the NaClManifestConnection object, and subsequent RPC handlers use
 * the connection object's reverse channel to forward RPC requests.
 */
int NaClManifestProxyCtor(struct NaClManifestProxy    *self,
                          NaClThreadIfFactoryFunction thread_factory_fn,
                          void                        *thread_factory_data,
                          struct NaClApp              *nap);

int NaClManifestProxyConnectionCtor(struct NaClManifestProxyConnection  *self,
                                    struct NaClManifestProxy            *server,
                                    struct NaClDesc                     *conn);

int NaClManifestProxyConnectionFactory(
    struct NaClSimpleService            *vself,
    struct NaClDesc                     *conn,
    struct NaClSimpleServiceConnection  **out);

extern struct NaClSimpleServiceConnectionVtbl
  const kNaClManifestProxyConnectionVtbl;

EXTERN_C_END

#endif
