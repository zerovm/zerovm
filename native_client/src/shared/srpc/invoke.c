/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl SRPC library.  A primitive rpc library.
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_macros.h"
#ifdef __native_client__
#include <inttypes.h>
#else
#include "native_client/src/trusted/desc/nacl_desc_imc.h"
#endif  /* __native_client__ */
#include "native_client/src/shared/srpc/nacl_srpc.h"
#include "native_client/src/shared/srpc/nacl_srpc_internal.h"


/*
 * Utility method for type checking argument lists.
 */
static int TypeCheckArgs(const char* arg_types, NaClSrpcArg** alist) {
  const char* p;

  for (p = arg_types; '\0' != *p && ':' != *p; ++p, ++alist) {
    if (NULL == *alist) {
      /* Too few arguments */
      return 0;
    }
    /* This code could be more compact by using a 256 entry table. */
    switch (*p) {
      case NACL_SRPC_ARG_TYPE_BOOL:
      case NACL_SRPC_ARG_TYPE_CHAR_ARRAY:
      case NACL_SRPC_ARG_TYPE_DOUBLE:
      case NACL_SRPC_ARG_TYPE_DOUBLE_ARRAY:
      case NACL_SRPC_ARG_TYPE_HANDLE:
      case NACL_SRPC_ARG_TYPE_INT:
      case NACL_SRPC_ARG_TYPE_INT_ARRAY:
      case NACL_SRPC_ARG_TYPE_LONG:
      case NACL_SRPC_ARG_TYPE_LONG_ARRAY:
      case NACL_SRPC_ARG_TYPE_STRING:
        if ((*alist)->tag != (enum NaClSrpcArgType) *p) {
          return 0;
        }
        break;
      /* These cases are added to avoid warnings.  */
      case NACL_SRPC_ARG_TYPE_OBJECT:
      case NACL_SRPC_ARG_TYPE_VARIANT_ARRAY:
      case NACL_SRPC_ARG_TYPE_INVALID:
      default:
        return 0;
    }
  }
  if (NULL != *alist) {
    /* Too many arguments */
    return 0;
  }
  return 1;
}

/*
 * Methods for invoking RPCs.
 */
NaClSrpcError NaClSrpcInvokeV(NaClSrpcChannel* channel,
                              uint32_t rpc_number,
                              NaClSrpcArg* args[],
                              NaClSrpcArg* rets[]) {
  int i;
  NaClSrpcRpc        rpc;
  NaClSrpcError      retval;
  const char*        rpc_name;
  const char*        arg_types;
  const char*        ret_types;

  if (NULL == channel) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcInvokeV: channel == NULL\n");
    return NACL_SRPC_RESULT_INTERNAL;
  }
  if (NaClSrpcServiceMethodNameAndTypes(channel->client,
                                        rpc_number,
                                        &rpc_name,
                                        &arg_types,
                                        &ret_types)) {
    /* Check input parameters for type conformance */
    if (!TypeCheckArgs(arg_types, args)) {
      NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                  "NaClSrpcInvokeV(channel=%p): input arg mismatch\n",
                  (void*) channel);
      return NACL_SRPC_RESULT_IN_ARG_TYPE_MISMATCH;
    }
    /* Check return values for type conformance */
    if (!TypeCheckArgs(ret_types, rets)) {
      NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                  "NaClSrpcInvokeV(channel=%p): output arg mismatch\n",
                  (void*) channel);
      return NACL_SRPC_RESULT_OUT_ARG_TYPE_MISMATCH;
    }
  } else {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcInvokeV(channel=%p): bad rpc number\n",
                (void*) channel);
    return NACL_SRPC_RESULT_BAD_RPC_NUMBER;
  }
  NaClSrpcLog(1,
              "NaClSrpcInvokeV: request(channel=%p, rpc_number=%"NACL_PRIu32
              ", rpc_name=\"%s\")\n",
              (void*) channel,
              rpc_number,
              rpc_name);

  for (i = 0; args[i] != NULL; i++ ) {
    char buffer[256];
    NaClSrpcFormatArg(2, args[i], buffer, NACL_ARRAY_SIZE(buffer));
    NaClSrpcLog(2,
                "NaClSrpcInvokeV: request(channel=%p, args[%d]=%s)\n",
                (void*) channel,
                i,
                buffer);
  }

  /*
   * First we send the request.
   * This requires sending args and the types and array sizes from rets.
   */
  rpc.protocol_version = kNaClSrpcProtocolVersion;
  rpc.rpc_number = rpc_number;
  rpc.request_id = channel->next_outgoing_request_id;
  rpc.result = NACL_SRPC_RESULT_OK;
  rpc.rets = rets;
  rpc.ret_types = ret_types;
  retval = NaClSrpcRequestWrite(channel, &rpc, args, rets);
  if (!retval) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcInvokeV(channel=%p): rpc request send failed\n",
                (void*) channel);
    return NACL_SRPC_RESULT_INTERNAL;
  }

  /* Then we wait for the response. */
  NaClSrpcRpcWait(channel, &rpc);
  NaClSrpcLog(1,
              "NaClSrpcInvokeV: response(channel=%p, rpc_number=%"NACL_PRIu32
              ", rpc_name=\"%s\", result=%d, string=\"%s\")\n",
              (void*) channel,
              rpc_number,
              rpc_name,
              rpc.result,
              NaClSrpcErrorString(rpc.result));

  for (i = 0; rets[i] != NULL; i++ ) {
    char buffer[256];
    NaClSrpcFormatArg(2, rets[i], buffer, NACL_ARRAY_SIZE(buffer));
    NaClSrpcLog(2,
                "NaClSrpcInvokeV: response(channel=%p, rets[%d]=%s)\n",
                (void*) channel,
                i,
                buffer);
  }

  return rpc.result;
}

/*
 * Parameter passing and return involves a significant amount of replication
 * that could be handled through templates.  What follows is a set of
 * macros for that task.
 */
/*
 * Some steps involve skipping a parameter in a va_arg list.
 */
#define SKIP(va, impl_type) \
    (void) va_arg(va, impl_type);

/*
 * The first phase is the args[] vector construction.
 */
#define SCALAR_ARG(arg, field, va, impl_type)     \
    (arg)->field = va_arg(va, impl_type)
#define ARRAY_ARG(arg, array_name, va, impl_type) \
    (arg)->u.count = va_arg(va, uint32_t);        \
    (arg)->array_name = va_arg(va, impl_type)
#define BOOL_ARG(arg, field, va, impl_type)       \
    (arg)->u.bval = (va_arg(va, impl_type) != 0)

/*
 * The second phase is the rets[] vector construction before invocation.
 */
#define SCALAR_RETINIT(arg, field, va, impl_type)     \
    (arg)->field = (impl_type) 0;                     \
    SKIP(va, impl_type *)
#define ARRAY_RETINIT(arg, array_name, va, impl_type) \
    (arg)->u.count = *va_arg(va, uint32_t*);          \
    (arg)->array_name = va_arg(va, impl_type)
#define BOOL_RETINIT(arg, field, va, impl_type)       \
    SKIP(va, impl_type *)

/*
 * The third phase is skipping the args[] after invocation.
 */
#define SCALAR_SKIP(arg, field, va, impl_type)     \
    SKIP(va, impl_type)
#define ARRAY_SKIP(arg, array_name, va, impl_type) \
    SKIP(va, uint32_t)                             \
    SKIP(va, impl_type)
#define BOOL_SKIP(arg, field, va, impl_type)       \
    SCALAR_SKIP(arg, field, va, impl_type)

/*
 * The fourth phase is copying the rets[] into the va_args after invocation.
 */
#define SCALAR_RET(arg, field, va, impl_type)     \
    *va_arg(va, impl_type *) = (arg)->field
#define ARRAY_RET(arg, array_name, va, impl_type) \
    *va_arg(va, uint32_t*) = (arg)->u.count;      \
    SKIP(va, impl_type)
#define BOOL_RET(arg, field, va, impl_type)       \
    *va_arg(va, impl_type *) = ((arg)->field != 0)

/*
 * All the phases consist of a loop around a switch enumerating types.
 */
#define ARGRET_SWITCH(phase, va, arg)                           \
    switch (*p) {                                               \
      case NACL_SRPC_ARG_TYPE_BOOL:                             \
        BOOL_##phase(arg, u.bval, va, int);                     \
        break;                                                  \
      case NACL_SRPC_ARG_TYPE_CHAR_ARRAY:                       \
        ARRAY_##phase(arg, arrays.carr, va, char*);             \
        break;                                                  \
      case NACL_SRPC_ARG_TYPE_DOUBLE:                           \
        SCALAR_##phase(arg, u.dval, va, double);                \
        break;                                                  \
      case NACL_SRPC_ARG_TYPE_DOUBLE_ARRAY:                     \
        ARRAY_##phase(arg, arrays.darr, va, double*);           \
        break;                                                  \
      case NACL_SRPC_ARG_TYPE_HANDLE:                           \
        SCALAR_##phase(arg, u.hval, va, NaClSrpcImcDescType);   \
        break;                                                  \
      case NACL_SRPC_ARG_TYPE_INT:                              \
        SCALAR_##phase(arg, u.ival, va, int32_t);               \
        break;                                                  \
      case NACL_SRPC_ARG_TYPE_INT_ARRAY:                        \
        ARRAY_##phase(arg, arrays.iarr, va, int32_t*);          \
        break;                                                  \
      case NACL_SRPC_ARG_TYPE_LONG:                             \
        SCALAR_##phase(arg, u.lval, va, int64_t);               \
        break;                                                  \
      case NACL_SRPC_ARG_TYPE_LONG_ARRAY:                       \
        ARRAY_##phase(arg, arrays.larr, va, int64_t*);          \
        break;                                                  \
      case NACL_SRPC_ARG_TYPE_STRING:                           \
        SCALAR_##phase(arg, arrays.str, va, char*);             \
        break;                                                  \
      /*                                                        \
       * The two cases below are added to avoid warnings,       \
       * they are only used  in the plugin code                 \
       */                                                       \
      case NACL_SRPC_ARG_TYPE_OBJECT:                           \
      case NACL_SRPC_ARG_TYPE_VARIANT_ARRAY:                    \
      default:                                                  \
        rv = NACL_SRPC_RESULT_APP_ERROR;                        \
        goto done;                                              \
    }

static void FreeArgs(NaClSrpcArg** vec) {
  if (NULL == vec) {
    return;
  }
  free(vec[0]);
  free(vec);
}

static NaClSrpcArg** AllocArgs(size_t vector_length) {
  NaClSrpcArg** vec;
  size_t vector_length_in_bytes;
  size_t i;

  /* Allocate the index vector. */
  if (NACL_SRPC_MAX_ARGS < vector_length) {
    return NULL;
  }
  vector_length_in_bytes = (vector_length + 1) * sizeof *vec;
  vec = (NaClSrpcArg **) malloc(vector_length_in_bytes);
  memset(vec, 0, vector_length_in_bytes);
  if (NULL == vec) {
    FreeArgs(vec);
    return NULL;
  }
  /* Allocate and initialize the arguments (if any). */
  if (0 != vector_length) {
    vec[0] = (NaClSrpcArg *) malloc(vector_length * sizeof *vec[0]);
    if (NULL == vec[0]) {
      FreeArgs(vec);
      return NULL;
    }
    for (i = 0; i < vector_length; ++i) {
      NaClSrpcArgCtor(vec[0] + i);
    }
  }
  /* Set the index vector to point to the arguments. */
  for (i = 1; i < vector_length; ++i) {
    vec[i] = vec[0] + i;
  }
  vec[vector_length] = 0;
  return vec;
}

NaClSrpcError NaClSrpcInvokeVaList(NaClSrpcChannel  *channel,
                                   uint32_t         rpc_num,
                                   va_list          in_va,
                                   va_list          out_va) {
  char const        *rpc_name;
  char const        *arg_types;
  char const        *ret_types;
  size_t            num_in;
  size_t            num_out;
  uint32_t          i;
  NaClSrpcArg       **inv = NULL;
  NaClSrpcArg       **outv = NULL;
  char const        *p;
  NaClSrpcError     rv;

  if (NULL == channel) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcInvokeVaList: channel == NULL\n");
    return NACL_SRPC_RESULT_INTERNAL;
  }

  if (!NaClSrpcServiceMethodNameAndTypes(channel->client,
                                         rpc_num,
                                         &rpc_name,
                                         &arg_types,
                                         &ret_types)) {
    /*
     * If rpc_number is out of range, this will return an error before
     * communicating with the server.
     */
    return NACL_SRPC_RESULT_BAD_RPC_NUMBER;
  }

  num_in = strlen(arg_types);
  num_out = strlen(ret_types);

  if (NACL_SRPC_MAX_ARGS < num_in || NACL_SRPC_MAX_ARGS < num_out) {
    return NACL_SRPC_RESULT_APP_ERROR;
  }

  rv = NACL_SRPC_RESULT_NO_MEMORY;
  inv = AllocArgs(num_in);
  if (NULL == inv) {
    goto done;
  }
  outv = AllocArgs(num_out);
  if (NULL == outv) {
    goto done;
  }

  for (i = 0, p = arg_types; i < num_in; ++i, ++p) {
    ARGRET_SWITCH(ARG, in_va, inv[i]);
    inv[i]->tag = (enum NaClSrpcArgType) *p;
  }
  for (i = 0, p = ret_types; i < num_out; ++i, ++p) {
    ARGRET_SWITCH(RETINIT, in_va, outv[i]);
    outv[i]->tag = (enum NaClSrpcArgType) *p;
  }

  rv = NaClSrpcInvokeV(channel, rpc_num, inv, outv);

  for (i = 0, p = arg_types; i < num_in; ++i, ++p) {
    ARGRET_SWITCH(SKIP, out_va, inv[i]);
  }
  for (i = 0, p = ret_types; i < num_out; ++i, ++p) {
    ARGRET_SWITCH(RET, out_va, outv[i]);
  }

 done:
  FreeArgs(outv);
  FreeArgs(inv);
  return rv;
}

NaClSrpcError NaClSrpcInvoke(NaClSrpcChannel  *channel,
                             uint32_t         rpc_num,
                             ...) {
  va_list       in_va;
  va_list       out_va;
  NaClSrpcError rv;

  va_start(in_va, rpc_num);
  va_start(out_va, rpc_num);

  rv = NaClSrpcInvokeVaList(channel, rpc_num, in_va, out_va);
  /*
   * Before the messages are sent to the server, rpc_num will be checked
   * for validity.
   */

  va_end(out_va);
  va_end(in_va);

  return rv;
}

NaClSrpcError NaClSrpcInvokeBySignature(NaClSrpcChannel  *channel,
                                        const char       *rpc_signature,
                                        ...) {
  uint32_t            rpc_num;
  va_list             in_va;
  va_list             out_va;
  NaClSrpcError       rv;

  if (NULL == channel) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcInvokeBySignature: channel == NULL\n");
    return NACL_SRPC_RESULT_INTERNAL;
  }
  rpc_num = NaClSrpcServiceMethodIndex(channel->client, rpc_signature);
  if (kNaClSrpcInvalidMethodIndex == rpc_num) {
    /*
     * kNaClSrpcInvalidMethodIndex is returned when rpc_name does not match
     * any method in the client service.  Explicitly check and return an error.
     */
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcInvokeBySignature(channel=%p):"
                "missing signature [%s]\n",
                (void*) channel,
                rpc_signature);
    return NACL_SRPC_RESULT_APP_ERROR;
  }

  va_start(in_va, rpc_signature);
  va_start(out_va, rpc_signature);

  rv = NaClSrpcInvokeVaList(channel, rpc_num, in_va, out_va);

  va_end(out_va);
  va_end(in_va);

  return rv;
}
