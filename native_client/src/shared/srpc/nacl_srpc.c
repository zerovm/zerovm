/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service library.  a primitive rpc library
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifndef __native_client__
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/desc/nacl_desc_imc.h"
#endif  /* __native_client__ */
#include "native_client/src/shared/srpc/nacl_srpc.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/srpc/nacl_srpc_internal.h"
#include "native_client/src/shared/srpc/nacl_srpc_message.h"

/*
 * Service discovery is used to build an interface description that
 * is searched for rpc dispatches.
 */
static NaClSrpcService* BuildClientService(NaClSrpcChannel* channel) {
  NaClSrpcArg*         ins[] = { NULL };
  NaClSrpcArg          out_carray;
  NaClSrpcArg*         outs[2];
  NaClSrpcService*     tmp_service = NULL;
  NaClSrpcService*     service = NULL;
  NaClSrpcHandlerDesc  basic_services[] = { { NULL, NULL } };
  NaClSrpcError        service_discovery_result;
  NaClSrpcService*     returned_service = NULL;

  /*
   * Service discovery requires a temporary setting of channel->client.
   * To ensure that this does not clobber a previous setting, enforce
   * a precondition that it is NULL beforehand.
   */
  CHECK(NULL == channel->client);
  /* Set up the output parameters for service discovery. */
  NaClSrpcArgCtor(&out_carray);
  outs[0] = &out_carray;
  outs[1] = NULL;
  out_carray.tag = NACL_SRPC_ARG_TYPE_CHAR_ARRAY;
  out_carray.u.count = NACL_SRPC_MAX_SERVICE_DISCOVERY_CHARS;
  /*
   * Add one guaranteed NUL byte to make sure the string will be NUL-terminated.
   */
  out_carray.arrays.carr = calloc(NACL_SRPC_MAX_SERVICE_DISCOVERY_CHARS + 1, 1);
  if (NULL == out_carray.arrays.carr) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "BuildClientService(channel=%p):"
                " service discovery malloc failed\n",
                (void*) channel);
    goto cleanup;
  }
  /* Set up the basic service descriptor to make the service discovery call. */
  tmp_service = (NaClSrpcService*) malloc(sizeof(*tmp_service));
  if (NULL == tmp_service) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "BuildClientService(channel=%p):"
                " temporary service malloc failed\n",
                (void*) channel);
    goto cleanup;
  }
  if (!NaClSrpcServiceHandlerCtor(tmp_service, basic_services)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "BuildClientService(channel=%p):"
                " NaClSrpcServiceHandlerCtor failed\n",
                (void*) channel);
    free(tmp_service);
    tmp_service = NULL;
    goto cleanup;
  }
  /* Temporarily assign channel->client to allow Invoke. */
  channel->client = tmp_service;
  /* Invoke service discovery, getting description string */
  service_discovery_result = NaClSrpcInvokeV(channel, 0, ins, outs);
  /* Disconnect the temporary service. */
  channel->client = NULL;
  if (NACL_SRPC_RESULT_OK != service_discovery_result) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "BuildClientService(channel=%p):"
                " service discovery invoke failed\n",
                (void*) channel);
    goto cleanup;
  }
  /* Allocate the real service. */
  service = (NaClSrpcService*) malloc(sizeof(*service));
  if (NULL == service) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "BuildClientService(channel=%p):"
                " service discovery malloc failed\n",
                (void*) channel);
    goto cleanup;
  }
  /* Build the real service from the resulting string. */
  if (!NaClSrpcServiceStringCtor(service, out_carray.arrays.carr)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "BuildClientService(channel=%p):"
                " NaClSrpcServiceStringCtor failed\n",
                (void*) channel);
    goto cleanup;
  }
  /* Caller takes ownership of service. */
  returned_service = service;
  service = NULL;
 cleanup:
  free(service);
  NaClSrpcServiceDtor(tmp_service);
  free(tmp_service);
  free(out_carray.arrays.carr);
  return returned_service;
}

/*
 * The constructors and destructor.
 * Clients and Servers are both NaClSrpcChannels, but they are constructed
 * by different methods reflecting how they get their table of rpc methods.
 */

void NaClSrpcChannelInitialize(NaClSrpcChannel* channel) {
  channel->message_channel = NULL;
  channel->next_outgoing_request_id = 0;
  channel->server = NULL;
  channel->client = NULL;
  channel->server_instance_data = NULL;
}

static int NaClSrpcChannelCtorHelper(NaClSrpcChannel* channel,
                                     NaClSrpcImcDescType handle) {
  NaClSrpcLog(1,
              "NaClSrpcChannelCtorHelper(channel=%p, handle=%p)\n",
              (void*) channel,
              (void*) handle);
  /* Initialize the channel to allow errors to be handled. */
  NaClSrpcChannelInitialize(channel);
  /* Create the message fragmentation layer. */
  channel->message_channel = NaClSrpcMessageChannelNew(handle);
  if (NULL == channel->message_channel) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcChannelCtorHelper(channel=%p):"
                " NaClSrpcMessageChannelNew failed\n",
                (void*) channel);
    return 0;
  }
  return 1;
}

static void NaClSrpcChannelDtorHelper(NaClSrpcChannel* channel) {
  NaClSrpcLog(1, "NaClSrpcChannelDtorHelper(channel=%p)\n", (void*) channel);
  channel->server_instance_data = NULL;
  channel->next_outgoing_request_id = 0;
  NaClSrpcMessageChannelDelete(channel->message_channel);
  channel->message_channel = NULL;
}

int NaClSrpcClientCtor(NaClSrpcChannel* channel, NaClSrpcImcDescType handle) {
  int return_value                = 0;
  NaClSrpcService* client_service = NULL;
  NaClSrpcLog(1,
              "NaClSrpcClientCtor(channel=%p, handle=%p)\n",
              (void*) channel,
              (void*) handle);
  if (!NaClSrpcChannelCtorHelper(channel, handle)) {
    goto cleanup;
  }
  /* Do service discovery to speed method invocation. */
  client_service = BuildClientService(channel);
  if (NULL == client_service) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcClientCtor(channel=%p):"
                " BuildClientService failed\n",
                (void*) channel);
    NaClSrpcChannelDtorHelper(channel);
    goto cleanup;
  }
  /* Channel takes ownership of client_service. */
  channel->client = client_service;
  client_service = NULL;
  return_value = 1;
 cleanup:
  NaClSrpcServiceDtor(client_service);
  return return_value;
}

int NaClSrpcServerCtor(NaClSrpcChannel* channel,
                       NaClSrpcImcDescType handle,
                       NaClSrpcService* service,
                       void* server_instance_data) {
  int return_value = 0;
  NaClSrpcLog(1,
              "NaClSrpcServerCtor(channel=%p, handle=%p,"
              " service=%p, server_instance_data=%p)\n",
              (void*) channel,
              (void*) handle,
              (void*) service,
              (void*) server_instance_data);
  if (!NaClSrpcChannelCtorHelper(channel, handle)) {
    goto cleanup;
  }
  /* Channel takes ownership of service. */
  channel->server = service;
  /* Channel does not take ownership of server_instance_data. */
  channel->server_instance_data = server_instance_data;
  return_value = 1;
 cleanup:
  return return_value;
}

void NaClSrpcDtor(NaClSrpcChannel* channel) {
  NaClSrpcLog(1,
              "NaClSrpcDtor(channel=%p)\n",
              (void*) channel);
  if (NULL == channel) {
    return;
  }
  channel->server_instance_data = NULL;
  NaClSrpcServiceDtor(channel->client);
  free(channel->client);
  channel->client = NULL;
  NaClSrpcServiceDtor(channel->server);
  free(channel->server);
  channel->server = NULL;
  /* Do the common part destruction. */
  NaClSrpcChannelDtorHelper(channel);
}

void NaClSrpcArgCtor(NaClSrpcArg* arg) {
  memset(arg, 0, sizeof *arg);
}

/*
 * A standalone SRPC server is not a subprocess of the browser or
 * sel_universal.  As this is a mode used for testing, the parent environment
 * must set the following variable to indicate that fact.
 */
const char kSrpcStandalone[] = "NACL_SRPC_STANDALONE";

int NaClSrpcIsStandalone() {
  return (NULL != getenv(kSrpcStandalone));
}
