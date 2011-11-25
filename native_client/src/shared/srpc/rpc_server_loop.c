/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/shared/srpc/nacl_srpc.h"
#include "native_client/src/shared/srpc/nacl_srpc_internal.h"

/*
 * ServerLoop processes the RPCs that this descriptor listens to.
 */
static int ServerLoop(NaClSrpcService* service,
                      NaClSrpcImcDescType socket_desc,
                      void* instance_data) {
  NaClSrpcChannel* channel = NULL;
  int retval = 0;

  NaClSrpcLog(2, "ServerLoop(service=%p, socket_desc=%p, instance_data=%p)\n",
              (void*) service,
              (void*) socket_desc,
              (void*) instance_data);
  /*
   * SrpcChannel objects can be large due to the buffers they contain.
   * Hence they should not be allocated on a thread stack.
   */
  channel = (NaClSrpcChannel*) malloc(sizeof(*channel));
  if (NULL == channel) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "ServerLoop: channel malloc failed\n");
    goto cleanup;
  }
  /* Create an SRPC server listening on the new file descriptor. */
  if (!NaClSrpcServerCtor(channel, socket_desc, service, instance_data)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "ServerLoop: NaClSrpcServerCtor failed\n");
    goto cleanup;
  }
  /*
   * Loop receiving RPCs and processing them.
   * The loop stops when a method requests a break out of the loop
   * or the IMC layer is unable to satisfy a request.
   */
  NaClSrpcRpcWait(channel, NULL);
  retval = 1;
  NaClSrpcLog(2,
              "ServerLoop(service=%p, socket_desc=%p, instance_data=%p) done\n",
              (void*) service,
              (void*) socket_desc,
              (void*) instance_data);

 cleanup:
  NaClSrpcDtor(channel);
  free(channel);
  return retval;
}

int NaClSrpcServerLoop(NaClSrpcImcDescType imc_socket_desc,
                       const NaClSrpcHandlerDesc methods[],
                       void* instance_data) {
  NaClSrpcService* service;

  /* Ensure we are passed a valid socket descriptor. */
  if (NACL_INVALID_DESCRIPTOR == imc_socket_desc) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcServerLoop: bad imc_socket_desc\n");
    return 0;
  }
  /* Allocate the service structure. */
  service = (struct NaClSrpcService*) malloc(sizeof(*service));
  if (NULL == service) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcServerLoop: service malloc failed\n");
    return 0;
  }
  /* Build the service descriptor. */
  if (!NaClSrpcServiceHandlerCtor(service, methods)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcServerLoop: NaClSrpcServiceHandlerCtor failed\n");
    free(service);
    return 0;
  }
  /* Process the RPCs.  ServerLoop takes ownership of service. */
  return ServerLoop(service, imc_socket_desc, instance_data);
}
