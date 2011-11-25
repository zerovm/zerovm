/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl simple RPC over IMC mechanism.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef __native_client__
#include <inttypes.h>
#include <nacl/nacl_inttypes.h>
#define NACL_ABI_EIO EIO
#define NACL_ABI_EINVAL EINVAL
#else
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#endif
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/srpc/nacl_srpc.h"
#include "native_client/src/shared/srpc/nacl_srpc_internal.h"
#include "native_client/src/shared/srpc/nacl_srpc_message.h"

#ifndef SIZE_T_MAX
# define SIZE_T_MAX (~((size_t) 0))
#endif

/*
 * Message formats:
 *
 * SRPC communicates using two message types, requests and responses.
 * Both are communicated with a header (NaClSrpcHeader below) prepended.
 *
 * Requests and responses also convey vectors of parameters that are
 * represented in memory by NaClSrpcArg.  Because NaClSrpcArgs may have a
 * non-fixed amount of additional memory associated (strings, arrays, etc.),
 * the form communicated separates the vectors of NaClSrpcArgs into a vector
 * of fixed length structs (ArgFixed) and a region of non-fixed memory.
 *
 * request:
 *   RpcHeader       header
 *   -- header info, value_len and template_len
 *      where value_len <= NACL_SRPC_MAX_ARGS, and
 *            template_len <= NACL_SRPC_MAX_ARGS
 *   ArgFixed        templates[template_len]
 *   -- conveys the types and sizes of rets
 *   ArgFixed        values[value_len]
 *   -- conveys the scalar values, and array types and sizes of args
 *   char            nonfixed[]
 *   -- the bytes used by strings, arrays, etc., in the elements of args in
 *      the order seen in args
 *
 * response:
 *   RpcHeader       header
 *   -- header info, value_len and template_len
 *      where value_len <= NACL_SRPC_MAX_ARGS, and template_len == 0
 *   ArgFixed        values[value_len]
 *   -- conveys the scalar values, and array types and sizes of rets
 *   char            nonfixed[]
 *   -- the bytes used by strings, arrays, etc., in the elements of rets in
 *      the order seen in rets
 */

#define HEADER_IOV_ENTRY_MAX     1
#define TEMPLATE_IOV_ENTRY_MAX   NACL_SRPC_MAX_ARGS
#define VALUE_IOV_ENTRY_MAX      NACL_SRPC_MAX_ARGS
#define NONFIXED_IOV_ENTRY_MAX   NACL_SRPC_MAX_ARGS
#define IOV_ENTRY_MAX            (HEADER_IOV_ENTRY_MAX + \
                                  TEMPLATE_IOV_ENTRY_MAX + \
                                  VALUE_IOV_ENTRY_MAX + \
                                  NONFIXED_IOV_ENTRY_MAX)

/*
 * RpcFixed is the fixed portion of NaClSrpcRpc serialized to the channel.
 */
struct RpcFixed {
  NACL_SRPC_RPC_SERIALIZED_FIELDS;
};
static const size_t kRpcSize = sizeof(struct RpcFixed);

/*
 * ArgFixed is the fixed portion of NaClSrpcArg serialized to the channel.
 */
struct ArgFixed {
  NACL_SRPC_ARG_SERIALIZED_FIELDS;
};
static const size_t kArgSize = sizeof(struct ArgFixed);

typedef enum {
  BoolFalse = 0,
  BoolTrue = 1,
} BoolValue;

static ssize_t ErrnoFromImcRet(ssize_t imc_ret) {
  if (0 > imc_ret) {
    return imc_ret;
  } else {
    return -NACL_ABI_EIO;
  }
}

static void AddIovEntry(void* base,
                        size_t length,
                        size_t max_iov_len,
                        struct NaClImcMsgIoVec* iov,
                        size_t* iov_len,
                        size_t* total_bytes) {
  if (length == 0) {
    return;
  }
  if (*iov_len >= max_iov_len) {
    return;
  }
  iov[*iov_len].base = base;
  iov[*iov_len].length = length;
  ++*iov_len;
  *total_bytes += length;
}

static nacl_abi_size_t VectorLen(NaClSrpcArg** vec) {
  nacl_abi_size_t len = 0;
  for (len = 0; len <= NACL_SRPC_MAX_ARGS; ++len) {
    if (vec[len] == NULL) {
      return len;
    }
  }
  return NACL_SRPC_MAX_ARGS;
}

static size_t ArrayElementSize(NaClSrpcArg* arg) {
  switch (arg->tag) {
    case NACL_SRPC_ARG_TYPE_BOOL:
    case NACL_SRPC_ARG_TYPE_DOUBLE:
    case NACL_SRPC_ARG_TYPE_HANDLE:
    case NACL_SRPC_ARG_TYPE_INT:
    case NACL_SRPC_ARG_TYPE_INVALID:
    case NACL_SRPC_ARG_TYPE_LONG:
    case NACL_SRPC_ARG_TYPE_OBJECT:
    case NACL_SRPC_ARG_TYPE_VARIANT_ARRAY:
      /* These are all scalar types. */
      return 0;

    case NACL_SRPC_ARG_TYPE_CHAR_ARRAY:
      return sizeof *arg->arrays.carr;
    case NACL_SRPC_ARG_TYPE_DOUBLE_ARRAY:
      return sizeof *arg->arrays.darr;
    case NACL_SRPC_ARG_TYPE_INT_ARRAY:
      return sizeof *arg->arrays.iarr;
    case NACL_SRPC_ARG_TYPE_LONG_ARRAY:
      return sizeof *arg->arrays.larr;
    case NACL_SRPC_ARG_TYPE_STRING:
      return sizeof *arg->arrays.str;
  }
  /* UNREACHED */
  return 0;
}

static void ClearTemplateStringLengths(NaClSrpcArg** vec,
                                       size_t vec_len) {
  size_t i;
  for (i = 0; i < vec_len; ++i) {
    if (vec[i]->tag == NACL_SRPC_ARG_TYPE_STRING) {
      vec[i]->u.count = 0;
    }
  }
}

/*
 * Adds the IOV entries for the fixed portion of the arguments in vec.
 */
static void AddFixed(NaClSrpcArg** vec,
                     size_t vec_len,
                     size_t max_iov_len,
                     struct NaClImcMsgIoVec* iov,
                     size_t* iov_len,
                     size_t* expected_bytes) {
  size_t i;
  for (i = 0; i < vec_len; ++i) {
    AddIovEntry(vec[i], kArgSize, max_iov_len, iov, iov_len, expected_bytes);
  }
}

/*
 * Copies the handles/descriptors from descs into the appropriately typed
 * elements of vec.
 */
static BoolValue GetHandles(NaClSrpcArg** vec,
                            size_t vec_len,
                            NaClSrpcImcDescType* descs,
                            size_t desc_len) {
  size_t i;
  size_t handle_index = 0;
  for (i = 0; i < vec_len; ++i) {
    if (vec[i]->tag != NACL_SRPC_ARG_TYPE_HANDLE) {
      continue;
    }
    if (handle_index >= desc_len) {
      return BoolFalse;
    }
    vec[i]->u.hval = descs[handle_index];
    ++handle_index;
  }
  return BoolTrue;
}

/*
 * Reads the header of the message to determine whether to call RecvRequest or
 * RecvResponse.
 */
static ssize_t SrpcPeekMessage(struct NaClSrpcMessageChannel* channel,
                               NaClSrpcRpc* rpc) {
  struct NaClImcMsgIoVec iov[1];
  const size_t kMaxIovLen = NACL_ARRAY_SIZE(iov);
  size_t iov_len;
  NaClSrpcMessageHeader header;
  size_t expected_bytes;
  ssize_t retval;

  iov_len = 0;
  expected_bytes = 0;
  AddIovEntry(rpc, kRpcSize, kMaxIovLen, iov, &iov_len, &expected_bytes);
  header.iov = iov;
  header.iov_length = NACL_ARRAY_SIZE(iov);
  header.NACL_SRPC_MESSAGE_HEADER_DESCV = NULL;
  header.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH = 0;
  retval = NaClSrpcMessageChannelPeek(channel, &header);
  /*
   * Check that the peek read the RPC and that the argument lengths are sane.
   */
  if (retval < (ssize_t) expected_bytes) {
    retval = ErrnoFromImcRet(retval);
  }
  else if (rpc->value_len > NACL_SRPC_MAX_ARGS ||
           rpc->template_len > NACL_SRPC_MAX_ARGS) {
    retval = -NACL_ABI_EIO;
  }
  return retval;
}

/*
 * Adds the IOV entries for the nonfixed portions of an argument vector to
 * read.  For receiving requests we need to allocate memory for the nonfixed
 * portions of both the inputs and results, but we do not read the results
 * vector's nonfixed portion.  For receiving responses we do not allocate
 * memory for the nonfixed portion (the caller has already done that), but we
 * read the nonfixed portion of results.
 * If it returns BoolFalse, some memory may have been allocated.  It is the
 * caller's responsibility to clean up in that case.
 */
static BoolValue AddNonfixedForRead(NaClSrpcArg** vec,
                                    size_t vec_len,
                                    size_t max_iov_len,
                                    BoolValue alloc_value,
                                    BoolValue read_value,
                                    struct NaClImcMsgIoVec* iov,
                                    size_t* iov_len,
                                    size_t* expected_bytes) {
  size_t i;

  /* Initialize the array pointers to allow cleanup if errors happen. */
  if (alloc_value) {
    for (i = 0; i < vec_len; ++i) {
      vec[i]->arrays.oval = NULL;
    }
  }

  for (i = 0; i < vec_len; ++i) {
    size_t count = vec[i]->u.count;
    void* base = 0;
    size_t element_size = ArrayElementSize(vec[i]);

    if (element_size == 0) {
      /* Skip fixed size arguments. */
      continue;
    }
    if (SIZE_T_MAX / element_size < count) {
      return BoolFalse;
    }
    base = vec[i]->arrays.oval;
    if (alloc_value) {
      base = malloc(element_size * count);
      if (base == 0) {
        return BoolFalse;
      }
      vec[i]->arrays.oval = base;
    }
    if (read_value) {
      AddIovEntry(base, element_size * count, max_iov_len, iov, iov_len,
                  expected_bytes);
    }
  }
  return BoolTrue;
}

static BoolValue AllocateArgs(NaClSrpcArg** arg_pointers, size_t length) {
  NaClSrpcArg* arg_array;
  nacl_abi_size_t i;

  /* Initialize the array pointers to allow cleanup if errors happen. */
  for (i = 0; i < length + 1; ++i) {
    arg_pointers[i] = NULL;
  }
  if (length == 0) {
    return BoolTrue;
  }

  if (SIZE_T_MAX / sizeof **arg_pointers < length) {
    return BoolFalse;
  }
  arg_array = (NaClSrpcArg*) malloc(length * sizeof **arg_pointers);
  if (arg_array == NULL) {
    return BoolFalse;
  }
  /* Initialize the individual args. */
  for (i = 0; i < length; ++i) {
    NaClSrpcArgCtor(&arg_array[i]);
  }
  /* Set each array pointer to point to the respective arg. */
  for (i = 0; i < length; ++i) {
    arg_pointers[i] = &arg_array[i];
  }
  return BoolTrue;
}

static void FreeArgs(NaClSrpcArg** vec) {
  nacl_abi_size_t i;
  NaClSrpcArg* args;

  if (vec == NULL) {
    return;
  }
  args = vec[0];

  for (i = 0; i <= NACL_SRPC_MAX_ARGS; ++i) {
    if (vec[i] == NULL) {
      break;
    }
    switch (vec[i]->tag) {
      case NACL_SRPC_ARG_TYPE_BOOL:
      case NACL_SRPC_ARG_TYPE_DOUBLE:
      case NACL_SRPC_ARG_TYPE_HANDLE:
      case NACL_SRPC_ARG_TYPE_INT:
      case NACL_SRPC_ARG_TYPE_INVALID:
      case NACL_SRPC_ARG_TYPE_LONG:
      case NACL_SRPC_ARG_TYPE_OBJECT:
      case NACL_SRPC_ARG_TYPE_VARIANT_ARRAY:
        break;

      case NACL_SRPC_ARG_TYPE_CHAR_ARRAY:
      case NACL_SRPC_ARG_TYPE_DOUBLE_ARRAY:
      case NACL_SRPC_ARG_TYPE_INT_ARRAY:
      case NACL_SRPC_ARG_TYPE_LONG_ARRAY:
      case NACL_SRPC_ARG_TYPE_STRING:
        free(vec[i]->arrays.oval);
        break;
    }
    vec[i] = NULL;
  }
  free(args);
}

static ssize_t RecvRequest(struct NaClSrpcMessageChannel* channel,
                           NaClSrpcRpc* rpc,
                           NaClSrpcArg** inputs,
                           NaClSrpcArg** results) {
  struct NaClImcMsgIoVec iov[IOV_ENTRY_MAX];
  const size_t kMaxIovLen = NACL_ARRAY_SIZE(iov);
  size_t iov_len;
  NaClSrpcMessageHeader header;
  NaClSrpcImcDescType descs[NACL_SRPC_MAX_ARGS];
  size_t expected_bytes;
  ssize_t retval;

  /*
   * SrpcPeekMessage should have been called before this function, and should
   * have populated rpc.  Make sure that rpc points to a sane header.
   */
  if (!rpc->is_request ||
      rpc->template_len > NACL_SRPC_MAX_ARGS ||
      rpc->value_len > NACL_SRPC_MAX_ARGS) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "RecvRequest: rpc header invalid: is_request %"NACL_PRIu32", "
                "template_len %"NACL_PRIu32", value_len %"NACL_PRIu32"\n",
                rpc->is_request,
                rpc->template_len,
                rpc->value_len);
    retval = -NACL_ABI_EINVAL;
    goto done;
  }
  /*
   * A request will contain two vectors of NaClSrpcArgs.  Set the index
   * pointers passed in to new argument vectors that will be filled during
   * the next peek.
   */
  if (!AllocateArgs(results, rpc->template_len) ||
      !AllocateArgs(inputs, rpc->value_len)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "RecvRequest: AllocateArgs failed\n");
    retval = -NACL_ABI_EINVAL;
    goto done;
  }

  /*
   * Having read the header we know how many elements each argument vector
   * contains.  The next peek reads the fixed portion of these argument vectors,
   * but cannot yet read the variable length portion, because we do not yet
   * know the counts of array types or strings.
   */
  iov_len = 0;
  expected_bytes = 0;
  AddIovEntry(rpc, kRpcSize, kMaxIovLen, iov, &iov_len, &expected_bytes);
  AddFixed(results, rpc->template_len, kMaxIovLen, iov, &iov_len,
           &expected_bytes);
  AddFixed(inputs, rpc->value_len, kMaxIovLen, iov, &iov_len, &expected_bytes);
  header.iov = iov;
  header.iov_length = (nacl_abi_size_t) iov_len;
  header.NACL_SRPC_MESSAGE_HEADER_DESCV = NULL;
  header.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH = 0;
  retval = NaClSrpcMessageChannelPeek(channel, &header);
  if (retval < (ssize_t) expected_bytes) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "RecvRequest:"
                "NaClSrpcMessageChannelPeek incomplete: expected %"
                NACL_PRIdS", got %"NACL_PRIdS"\n",
                expected_bytes,
                retval);
    retval = ErrnoFromImcRet(retval);
    goto done;
  }

  /*
   * After peeking the fixed portion of the argument vectors we are ready to
   * read the nonfixed portions as well.  So the read just adds the IOV entries
   * for the nonfixed portions of the arguments.
   */
  iov_len = 0;
  expected_bytes = 0;
  AddIovEntry(rpc, kRpcSize, kMaxIovLen, iov, &iov_len, &expected_bytes);
  ClearTemplateStringLengths(results, rpc->template_len);
  AddFixed(results, rpc->template_len, kMaxIovLen, iov, &iov_len,
           &expected_bytes);
  AddFixed(inputs, rpc->value_len, kMaxIovLen, iov, &iov_len, &expected_bytes);
  if (!AddNonfixedForRead(results, rpc->template_len, kMaxIovLen,
                          1, 0, iov, &iov_len, &expected_bytes)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "RecvRequest: AllocateArgs failed for results\n");
    retval = -NACL_ABI_EIO;
    goto done;
  }
  if (!AddNonfixedForRead(inputs, rpc->value_len, kMaxIovLen,
                          1, 1, iov, &iov_len, &expected_bytes)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "RecvRequest: AllocateArgs failed for inputs\n");
    retval = -NACL_ABI_EIO;
    goto done;
  }
  header.iov = iov;
  header.iov_length = (nacl_abi_size_t) iov_len;
  header.NACL_SRPC_MESSAGE_HEADER_DESCV = descs;
  header.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH = NACL_ARRAY_SIZE(descs);
  retval = NaClSrpcMessageChannelReceive(channel, &header);
  if (retval < (ssize_t) expected_bytes) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "RecvRequest:"
                " NaClSrpcMessageChannelReceive incomplete: expected %"
                NACL_PRIdS", got %"NACL_PRIdS"\n",
                expected_bytes,
                retval);
    retval = ErrnoFromImcRet(retval);
    goto done;
  }

  /*
   * The read left any descriptors passed in the descs array.  We need to
   * copy those descriptors to the inputs vector.
   */
  if (!GetHandles(inputs, rpc->value_len,
                  descs, header.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "RecvRequest: GetHandles failed\n");
    retval = -NACL_ABI_EIO;
    goto done;
  }
  /*
   * Success, the caller has taken ownership of the memory we allocated
   * for inputs and results.
   */
  inputs = NULL;
  results = NULL;

 done:
  FreeArgs(inputs);
  FreeArgs(results);
  return retval;
}

/*
 * Checks for type conformance between a peeked results vector and the caller's
 * results vector.  If the types agree then nonfixed sizes are checked to
 * ensure that the caller's buffers are large enough to receive the peeked
 * nonfixed size objects.  If this agrees, then the caller's size is updated
 * to reflect how many elements were actually received.
 */
static BoolValue CheckMatchAndCopyCounts(size_t vec_len,
                                         NaClSrpcArg** expected,
                                         NaClSrpcArg** peeked) {
  size_t i;

  for (i = 0; i < vec_len; ++i) {
    if (expected[i]->tag != peeked[i]->tag) {
      return BoolFalse;
    }
    switch (expected[i]->tag) {
      case NACL_SRPC_ARG_TYPE_BOOL:
      case NACL_SRPC_ARG_TYPE_DOUBLE:
      case NACL_SRPC_ARG_TYPE_HANDLE:
      case NACL_SRPC_ARG_TYPE_INT:
      case NACL_SRPC_ARG_TYPE_INVALID:
      case NACL_SRPC_ARG_TYPE_LONG:
      case NACL_SRPC_ARG_TYPE_OBJECT:
      case NACL_SRPC_ARG_TYPE_VARIANT_ARRAY:
        break;

      case NACL_SRPC_ARG_TYPE_STRING:
        /*
         * Returned strings are allocated by the SRPC transport mechanism,
         * whereas all the other "array" types are allocated by the caller
         * of the respective Invoke routine.
         */
        expected[i]->arrays.oval = malloc(peeked[i]->u.count);
        if (expected[i]->arrays.oval == NULL) {
          return BoolFalse;
        }
        expected[i]->u.count = peeked[i]->u.count;
        break;

      case NACL_SRPC_ARG_TYPE_CHAR_ARRAY:
      case NACL_SRPC_ARG_TYPE_DOUBLE_ARRAY:
      case NACL_SRPC_ARG_TYPE_INT_ARRAY:
      case NACL_SRPC_ARG_TYPE_LONG_ARRAY:
        if (peeked[i]->u.count > expected[i]->u.count) {
          return BoolFalse;
        }
        expected[i]->u.count = peeked[i]->u.count;
        break;
    }
  }
  return BoolTrue;
}

static ssize_t RecvResponse(struct NaClSrpcMessageChannel* channel,
                            NaClSrpcRpc* rpc,
                            NaClSrpcArg** results) {
  NaClSrpcArg* result_copy[NACL_SRPC_MAX_ARGS + 1];
  struct NaClImcMsgIoVec iov[IOV_ENTRY_MAX];
  const size_t kMaxIovLen = NACL_ARRAY_SIZE(iov);
  size_t iov_len = 0;
  NaClSrpcMessageHeader header;
  NaClSrpcImcDescType descs[NACL_SRPC_MAX_ARGS];
  size_t expected_bytes;
  ssize_t retval;
  size_t i;

  if (results == NULL) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "RecvResponse: results should not be NULL\n");
    retval = -NACL_ABI_EINVAL;
    goto done;
  }
  /*
   * SrpcPeekMessage should have been called before this function, and should
   * have populated rpc.  Make sure that rpc points to a sane header.
   */
  if (rpc->is_request ||
      rpc->template_len > 0 ||
      rpc->value_len > NACL_SRPC_MAX_ARGS ||
      rpc->value_len != VectorLen(results)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "RecvResponse: rpc header invalid: is_request %"NACL_PRIu32", "
                "template_len %"NACL_PRIu32", value_len %"NACL_PRIu32"\n",
                rpc->is_request,
                rpc->template_len,
                rpc->value_len);
    return -NACL_ABI_EINVAL;
  }

  /*
   * Having read the header we know how many elements the results vector
   * contains.  The next peek reads the fixed portion of the results vectors,
   * but cannot yet read the variable length portion, because we do not yet
   * know the counts of array types or strings.  Because the results read
   * could conflict with the expected types, we need to read the fixed portion
   * into a copy.
   */
  if (!AllocateArgs(result_copy, rpc->value_len)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "RecvResponse: AllocateArgs failed\n");
    retval = -NACL_ABI_EINVAL;
    goto done;
  }
  iov_len = 0;
  expected_bytes = 0;
  AddIovEntry(rpc, kRpcSize, kMaxIovLen, iov, &iov_len, &expected_bytes);
  for (i = 0; i < rpc->value_len; ++i) {
    AddIovEntry(result_copy[i], kArgSize, kMaxIovLen, iov, &iov_len,
                &expected_bytes);
  }
  header.iov = iov;
  header.iov_length = (nacl_abi_size_t) iov_len;
  header.NACL_SRPC_MESSAGE_HEADER_DESCV = NULL;
  header.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH = 0;
  retval = NaClSrpcMessageChannelPeek(channel, &header);
  if (retval < (ssize_t) expected_bytes) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "RecvResponse: NaClSrpcMessageChannelPeek incomplete: "
                "expected %"NACL_PRIdS", got %"NACL_PRIdS"\n",
                expected_bytes,
                retval);
    retval = ErrnoFromImcRet(retval);
    goto done;
  }

  /*
   * Check that the peeked results vector's types conform to the types passed
   * in and that any nonfixed size arguments are no larger than the counts
   * passed in from the caller.  If the values are acceptable, we copy the
   * actual sizes to the caller's vector.
   */
  if (!CheckMatchAndCopyCounts(rpc->value_len, results, result_copy)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "RecvResponse: CheckMatchAndCopyCounts failed\n");
    retval = -NACL_ABI_EIO;
    goto done;
  }

  /*
   * After peeking the fixed portion of the results vector we are ready to
   * read the nonfixed portion as well.  So the read just adds the IOV entries
   * for the nonfixed portion of results.
   */
  iov_len = 0;
  expected_bytes = 0;
  AddIovEntry(rpc, kRpcSize, kMaxIovLen, iov, &iov_len, &expected_bytes);
  AddFixed(results, rpc->value_len, kMaxIovLen, iov, &iov_len, &expected_bytes);
  if (!AddNonfixedForRead(results, rpc->value_len, kMaxIovLen,
                          0, 1, iov, &iov_len, &expected_bytes)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "RecvResponse: AddNonfixedForRead failed\n");
    retval = -NACL_ABI_EIO;
    goto done;
  }
  header.iov = iov;
  header.iov_length = (nacl_abi_size_t) iov_len;
  header.NACL_SRPC_MESSAGE_HEADER_DESCV = descs;
  header.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH = NACL_ARRAY_SIZE(descs);
  retval = NaClSrpcMessageChannelReceive(channel, &header);
  if (retval < (ssize_t) expected_bytes) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "RecvResponse: NaClSrpcMessageChannelReceive incomplete: "
                "expected %"NACL_PRIdS", got %"NACL_PRIdS"\n",
                expected_bytes,
                retval);
    retval = ErrnoFromImcRet(retval);
    goto done;
  }

  /*
   * The read left any descriptors returned in the descs array.  We need to
   * copy those descriptors to the results vector.
   */
  if (!GetHandles(results, rpc->value_len,
                  descs, header.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "RecvResponse: GetHandles failed\n");
    retval = -NACL_ABI_EIO;
  }

 done:
  FreeArgs(result_copy);
  return retval;
}

/*
 * Adds IOV entries for the non-fixed portions of the arguments in vec.
 * This could also add handles/descriptors to the array to send.
 */
static BoolValue AddNonfixedForWrite(NaClSrpcArg** vec,
                                     size_t vec_len,
                                     size_t max_iov_len,
                                     struct NaClImcMsgIoVec* iov,
                                     size_t* iov_len,
                                     NaClSrpcImcDescType* descs,
                                     size_t* desc_len,
                                     size_t* expected_bytes) {
  size_t i;
  size_t element_size;
  nacl_abi_size_t count;
  void* base;

  /* Add IOV entries for the array/string types in vec. */
  for (i = 0; i < vec_len; ++i) {
    switch (vec[i]->tag) {
      case NACL_SRPC_ARG_TYPE_BOOL:
      case NACL_SRPC_ARG_TYPE_DOUBLE:
      case NACL_SRPC_ARG_TYPE_INT:
      case NACL_SRPC_ARG_TYPE_INVALID:
      case NACL_SRPC_ARG_TYPE_LONG:
      case NACL_SRPC_ARG_TYPE_OBJECT:
      case NACL_SRPC_ARG_TYPE_VARIANT_ARRAY:
       /* Scalar types are handled by fixed iovs alone. */
       break;

      case NACL_SRPC_ARG_TYPE_HANDLE:
        /* Handles are added into the desc array. */
        descs[*desc_len] = vec[i]->u.hval;
        ++*desc_len;
        break;

      case NACL_SRPC_ARG_TYPE_CHAR_ARRAY:
      case NACL_SRPC_ARG_TYPE_DOUBLE_ARRAY:
      case NACL_SRPC_ARG_TYPE_INT_ARRAY:
      case NACL_SRPC_ARG_TYPE_LONG_ARRAY:
        count = vec[i]->u.count;
        base = vec[i]->arrays.oval;
        element_size = ArrayElementSize(vec[i]);
        /* Check that computing the number of bytes will not overflow. */
        if (SIZE_T_MAX / element_size < count) {
          return BoolFalse;
        }
        AddIovEntry(base, element_size * count, max_iov_len, iov, iov_len,
                    expected_bytes);
        break;

      case NACL_SRPC_ARG_TYPE_STRING:
        count = (nacl_abi_size_t) strlen(vec[i]->arrays.str) + 1;
        base = vec[i]->arrays.oval;
        vec[i]->u.count = count;
        AddIovEntry(base, count, max_iov_len, iov, iov_len, expected_bytes);
        break;
    }
  }
  return BoolTrue;
}

static ssize_t SrpcSendMessage(NaClSrpcRpc* rpc,
                               NaClSrpcArg** inputs,
                               NaClSrpcArg** results,
                               struct NaClSrpcMessageChannel* channel) {
  NaClSrpcArg** values;
  struct NaClImcMsgIoVec iov[IOV_ENTRY_MAX];
  const size_t kMaxIovLen = NACL_ARRAY_SIZE(iov);
  size_t iov_len;
  NaClSrpcImcDescType descs[NACL_SRPC_MAX_ARGS];
  size_t desc_len = 0;
  NaClSrpcMessageHeader header;
  ssize_t retval;
  size_t expected_bytes;

  /*
   * The message will be sent in three portions:
   * 1) the header (rpc)
   * 2) the fixed portions of the inputs and results arrays (inputs may be
   *    null for responses)
   * 3) the nonfixed portions of the inputs or results array
   */
  iov_len = 0;
  expected_bytes = 0;
  AddIovEntry(rpc, kRpcSize, kMaxIovLen, iov, &iov_len, &expected_bytes);

  if (rpc->is_request) {
    /*
     * For requests we pass only the fixed portion of the results vector.
     * This is to convey to the invoked procedure the type and size of the
     * allowed result values.
     * The values to be passed in both fixed and nonfixed are from inputs.
     */
    if (results == NULL) {
      rpc->template_len = 0;
    } else {
      rpc->template_len = VectorLen(results);
      AddFixed(results, rpc->template_len, kMaxIovLen, iov, &iov_len,
               &expected_bytes);
    }
    values = inputs;
  } else {
    /*
     * For responses there are no templates, and the values that will be passed
     * are from results.
     */
    values = results;
    rpc->template_len = 0;
  }
  /*
   * Pass the fixed and nonfixed portions.
   */
  if (values == NULL) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "SrpcSendMessage: values should not be NULL\n");
    return -NACL_ABI_EINVAL;
  }
  rpc->value_len = VectorLen(values);
  AddFixed(values, rpc->value_len, kMaxIovLen, iov, &iov_len, &expected_bytes);
  if (!AddNonfixedForWrite(values, rpc->value_len,
                           kMaxIovLen,
                           iov, &iov_len,
                           descs, &desc_len,
                           &expected_bytes)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "SrpcSendMessage: AddNonfixedForWrite failed\n");
    return -NACL_ABI_EIO;
  }
  header.iov = iov;
  header.iov_length = (nacl_abi_size_t) iov_len;
  header.NACL_SRPC_MESSAGE_HEADER_DESCV = descs;
  header.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH = (nacl_abi_size_t) desc_len;
  retval = NaClSrpcMessageChannelSend(channel, &header);
  if (retval >= 0  && retval < (ssize_t) expected_bytes) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "SrpcSendMessage: NaClSrpcMessageChannelSend incomplete: "
                "expected %"NACL_PRIdS", got %"NACL_PRIdS"\n",
                expected_bytes,
                retval);
    return -NACL_ABI_EIO;
  }
  return retval;
}

/* TODO(sehr): make this public when the client side uses RPC. */
static BoolValue NaClSrpcRpcCtor(NaClSrpcRpc* rpc, NaClSrpcChannel* channel) {
  rpc->channel = channel;
  rpc->result = NACL_SRPC_RESULT_INTERNAL;
  rpc->rets = NULL;
  return BoolTrue;
}

/* A self-deleting closure to send responses from RPC servers. */
typedef struct RpcCheckingClosure {
  struct NaClSrpcClosure base;
  NaClSrpcRpc* rpc;
} RpcCheckingClosure;

static void RpcCheckingClosureRun(NaClSrpcClosure* self) {
  RpcCheckingClosure* vself = (RpcCheckingClosure*) self;
  NaClSrpcRpc* rpc = vself->rpc;
  ssize_t retval;

  do {
    const char* rpc_name;
    const char* arg_types;
    const char* ret_types;
    int i;
    NaClSrpcServiceMethodNameAndTypes(rpc->channel->server,
                                      rpc->rpc_number,
                                      &rpc_name,
                                      &arg_types,
                                      &ret_types);
    NaClSrpcLog(1,
                "RpcCheckingClosureRun: response(channel=%p,"
                " rpc_number=%"NACL_PRIu32
                ", rpc_name=\"%s\", result=%d, string=\"%s\")\n",
                (void*) rpc->channel,
                rpc->rpc_number,
                rpc_name,
                rpc->result,
                NaClSrpcErrorString(rpc->result));
    for (i = 0; rpc->rets[i] != NULL; i++ ) {
      char buffer[256];
      NaClSrpcFormatArg(2, rpc->rets[i], buffer, NACL_ARRAY_SIZE(buffer));
      NaClSrpcLog(2,
                  "RpcCheckingClosureRun: response(channel=%p, rets[%d]=%s)\n",
                  (void*) rpc->channel,
                  i,
                  buffer);
    }
  } while(0);
  /* Send the RPC response to the caller. */
  rpc->is_request = 0;
  rpc->dispatch_loop_should_continue = 1;
  if (NACL_SRPC_RESULT_BREAK == rpc->result) {
    NaClSrpcLog(2,
                "RpcCheckingClosureRun: server requested break\n");
    rpc->result = NACL_SRPC_RESULT_OK;
    rpc->dispatch_loop_should_continue = 0;
  }
  retval = SrpcSendMessage(rpc, NULL, rpc->rets, rpc->channel->message_channel);
  if (retval < 0) {
    /* If the response write failed, drop request and continue. */
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "RpcCheckingClosureRun: response write failed\n");
  }
  free(self);
}

static BoolValue RpcCheckingClosureCtor(RpcCheckingClosure* self,
                                        NaClSrpcRpc* rpc) {
  self->base.Run = RpcCheckingClosureRun;
  self->rpc = rpc;
  self->rpc->dispatch_loop_should_continue = 1;
  return BoolTrue;
}

static BoolValue RequestVectorTypesConform(NaClSrpcChannel* channel,
                                           NaClSrpcRpc* rpc,
                                           NaClSrpcArg** args,
                                           NaClSrpcArg** rets) {
  const char* rpc_name;
  const char* arg_types;
  const char* ret_types;
  ssize_t retval;
  size_t i;

  if (rpc->value_len > NACL_SRPC_MAX_ARGS ||
      rpc->template_len > NACL_SRPC_MAX_ARGS) {
    return BoolFalse;
  }
  /* Get service discovery's remembered types for args and rets */
  retval = NaClSrpcServiceMethodNameAndTypes(channel->server,
                                             rpc->rpc_number,
                                             &rpc_name,
                                             &arg_types,
                                             &ret_types);
  if (!retval) {
    return BoolFalse;
  }
  /* Check that lengths match. */
  if (rpc->value_len != strlen(arg_types) ||
      rpc->template_len != strlen(ret_types)) {
    return BoolFalse;
  }
  /* Check args for type conformance. */
  for (i = 0; i < rpc->value_len; ++i) {
    if (args[i] == NULL) {
      return BoolFalse;
    }
    if (args[i]->tag != (unsigned char) arg_types[i]) {
      return BoolFalse;
    }
  }
  if (args[rpc->value_len] != NULL) {
    return BoolFalse;
  }
  /* Check rets for type conformance. */
  for (i = 0; i < rpc->template_len; ++i) {
    if (rets[i] == NULL) {
      return BoolFalse;
    }
    if (rets[i]->tag != (unsigned char) ret_types[i]) {
      return BoolFalse;
    }
  }
  if (rets[rpc->template_len] != NULL) {
    return BoolFalse;
  }
  return BoolTrue;
}

/*
 * The receive/dispatch function returns an enum indicating how the enclosing
 * loop should proceed.
 */
typedef enum {
  DISPATCH_CONTINUE,  /* Continue receive-dispatch loop */
  DISPATCH_BREAK,     /* Break out of loop was requested by invoked method */
  DISPATCH_RESPONSE,  /* Instead of a request, we received a response */
  DISPATCH_EOF        /* No more requests or responses can be received */
} DispatchReturn;

static DispatchReturn NaClSrpcReceiveAndDispatch(NaClSrpcChannel* channel,
                                                 NaClSrpcRpc* rpc_stack_top) {
  NaClSrpcRpc rpc;
  NaClSrpcArg* args[NACL_SRPC_MAX_ARGS + 1];
  NaClSrpcArg* rets[NACL_SRPC_MAX_ARGS + 1];
  NaClSrpcMethod method;
  ssize_t bytes_read;
  RpcCheckingClosure* closure = NULL;
  /* DISPATCH_EOF is the closest we have to an error return. */
  DispatchReturn dispatch_return = DISPATCH_EOF;

  closure = (RpcCheckingClosure*) malloc(sizeof *closure);
  if (NULL == closure) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcReceiveAndDispatch(channel=%p):"
                "closure malloc failed\n",
                (void*) channel);
    dispatch_return = DISPATCH_EOF;
    goto done;
  }
  if (!NaClSrpcRpcCtor(&rpc, channel) ||
      !RpcCheckingClosureCtor(closure, &rpc)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcReceiveAndDispatch(channel=%p): constructor failed\n",
                (void*) channel);
    goto done;
  }
  rpc.rets = rets;
  /* Read a message from the channel. */
  bytes_read = SrpcPeekMessage(channel->message_channel, &rpc);
  if (bytes_read < 0) {
    goto done;
  }
  if (rpc.is_request) {
    /* This is a new request. */
    if (NULL == channel->server) {
      if (NULL == rpc_stack_top) {
        /* There is no service to dispatch requests. Abort. */
        dispatch_return = DISPATCH_EOF;
        goto done;
      } else {
        /* Inform the pending invoke that a failure happened. */
        NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                    "NaClSrpcReceiveAndDispatch(channel=%p):"
                    "out of order request\n",
                    (void*) channel);
        rpc_stack_top->result = NACL_SRPC_RESULT_INTERNAL;
        dispatch_return = DISPATCH_BREAK;
        goto done;
      }
    }
    /* Fall through to request handling below. */
  } else {
    /* This is a response to a pending request. */
    if (NULL == rpc_stack_top) {
      NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                  "NaClSrpcReceiveAndDispatch(channel=%p):"
                  "response, no pending request\n",
                  (void*) channel);
      /* There is no pending request. Abort. */
      dispatch_return = DISPATCH_BREAK;
      goto done;
    } else {
      if (rpc.request_id == rpc_stack_top->request_id) {
        /* Copy the serialized portion of the Rpc and process as a response. */
        memcpy(rpc_stack_top, &rpc, kRpcSize);
        dispatch_return = DISPATCH_RESPONSE;
        goto done;
      } else {
        /* Received an out-of-order response.  Abort. */
        NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                    "NaClSrpcReceiveAndDispatch(channel=%p):"
                    " response for wrong request\n",
                    (void*) channel);
        dispatch_return = DISPATCH_BREAK;
        goto done;
      }
    }
  }
  bytes_read = RecvRequest(channel->message_channel, &rpc, args, rets);
  if (bytes_read < 0) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcReceiveAndDispatch(channel=%p):"
                "RecvRequest failed, bytes=%"NACL_PRIdS"\n",
                (void*) channel,
                bytes_read);
    dispatch_return = DISPATCH_EOF;
  }
  if (!RequestVectorTypesConform(channel, &rpc, args, rets)) {
    NaClSrpcLog(NACL_SRPC_LOG_WARNING,
                "NaClSrpcReceiveAndDispatch(channel=%p):"
                " arg/ret type mismatch (recoverable error)\n",
                (void*) channel);
    dispatch_return = DISPATCH_CONTINUE;
    goto done;
  }
  /* Then we invoke the method, which computes a return code. */
  method = NaClSrpcServiceMethod(channel->server, rpc.rpc_number);
  if (NULL == method) {
    NaClSrpcLog(NACL_SRPC_LOG_WARNING,
                "NaClSrpcReceiveAndDispatch(channel=%p):"
                " bad rpc_number %"NACL_PRIu32" (recoverable error)\n",
                (void*) channel,
                rpc.rpc_number);
    dispatch_return = DISPATCH_CONTINUE;
    goto done;
  }
  do {
    const char* rpc_name;
    const char* arg_types;
    const char* ret_types;
    int i;
    NaClSrpcServiceMethodNameAndTypes(channel->server,
                                      rpc.rpc_number,
                                      &rpc_name,
                                      &arg_types,
                                      &ret_types);
    NaClSrpcLog(1, "NaClSrpcReceiveAndDispatch:"
                " request(channel=%p, rpc_number=%"NACL_PRIu32
                ", rpc_name=\"%s\")\n",
                (void*) channel,
                rpc.rpc_number,
                rpc_name);
    for (i = 0; args[i] != NULL; i++ ) {
      char buffer[256];
      NaClSrpcFormatArg(2, args[i], buffer, NACL_ARRAY_SIZE(buffer));
      NaClSrpcLog(2,
                  "NaClSrpcReceiveAndDispatch:"
                  " request(channel=%p, args[%d]=%s)\n",
                  (void*) channel,
                  i,
                  buffer);
    }
  } while(0);
  (*method)(&rpc, args, rets, (NaClSrpcClosure*) closure);
  FreeArgs(args);
  FreeArgs(rets);
  /* The invoked method takes ownership of the closure and deletes it. */
  closure = NULL;
  /*
   * Return code to either continue or break out of the processing loop.
   * When we separate closure invocation from the dispatch loop we will
   * have to implement a barrier to make sure that all preceding RPCs are
   * completed, and then signal the dispatcher to stop.
   */
  if (rpc.dispatch_loop_should_continue) {
    dispatch_return = DISPATCH_CONTINUE;
  } else {
    dispatch_return = DISPATCH_BREAK;
  }

 done:
  free(closure);
  return dispatch_return;
}

/*
 * After an RPC is sent, the current thread can block waiting for the
 * response.  As currently there is only one thread receiving messages,
 * this thread will handle receiving and dispatching while waiting for
 * its response.  This allows for one thread to process calls back and
 * forth between client and server, as required for the NPAPI main thread
 * in Pepper.
 */
void NaClSrpcRpcWait(NaClSrpcChannel* channel,
                     NaClSrpcRpc* rpc) {
  DispatchReturn retval;

  /*
   * Loop receiving RPCs and processing them.
   * The loop stops when the receive/dispatch function returns.
   */
  do {
    retval = NaClSrpcReceiveAndDispatch(channel, rpc);
  } while (DISPATCH_CONTINUE == retval);
  /* Process responses */
  NaClSrpcLog(2,
              "NaClSrpcRpcWait(channel=%p): loop done: %p, %d\n",
              (void*) channel,
              (void*) rpc,
              retval);
  if (NULL == rpc) {
    NaClSrpcLog(2,
                "NaClSrpcRpcWait(channel=%p):"
                " rpc is NULL (this is not an error)\n",
                (void*) channel);
    return;
  }
  if (DISPATCH_RESPONSE == retval) {
    ssize_t recv_ret = RecvResponse(channel->message_channel, rpc, rpc->rets);
    if (recv_ret < 0) {
      NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                  "NaClSrpcRpcWait(channel=%p): rpc receive failed (%"
                  NACL_PRIdS")\n",
                  (void*) channel,
                  recv_ret);
      rpc->result = NACL_SRPC_RESULT_INTERNAL;
      return;
    }
  } else if (DISPATCH_EOF == retval) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcRpcWait(channel=%p):"
                " EOF is received instead of response. Probably, the other"
                " side (usually, nacl module or browser plugin) crashed.\n",
                (void*) channel);
    rpc->result = NACL_SRPC_RESULT_INTERNAL;
  }
}

int NaClSrpcRequestWrite(NaClSrpcChannel* channel,
                         NaClSrpcRpc* rpc,
                         NaClSrpcArg** args,
                         NaClSrpcArg** rets) {
  ssize_t retval;
  rpc->is_request = 1;
  retval = SrpcSendMessage(rpc, args, rets, channel->message_channel);
  if (retval < 0) {
    /* Requests with bad handles could fail.  Report to the caller. */
    NaClSrpcLog(1,
                "NaClSrpcRequestWrite(channel=%p, retval=%"NACL_PRIu32
                ") failed\n",
                (void*) channel,
                rpc->rpc_number);
    return 0;
  }
  return 1;
}
