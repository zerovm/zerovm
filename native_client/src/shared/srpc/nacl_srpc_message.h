/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaClSrpcMessageChannel -- a buffered API over IMC sockets.
 */

#ifndef NATIVE_CLIENT_SRC_SHARED_SRPC_NACL_SRPC_MESSAGE_H_
#define NATIVE_CLIENT_SRC_SHARED_SRPC_NACL_SRPC_MESSAGE_H_
#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

#ifdef __native_client__
#include <sys/types.h>
#include <sys/nacl_imc_api.h>
#else  /* __native_client__ */
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/desc/nrd_xfer.h"
#endif  /* __native_client__ */

/*
 * A typedef for an IMC socket usable in trusted or untrusted code.
 */
#ifdef __native_client__
typedef int NaClSrpcMessageDesc;
#else  /* __native_client__ */
typedef struct NaClDesc* NaClSrpcMessageDesc;
#endif  /* __native_client__ */

/*
 * Create a NaClSrpcMessageChannel from the specified descriptor.  Takes
 * ownership of desc.
 */
struct NaClSrpcMessageChannel* NaClSrpcMessageChannelNew(
    NaClSrpcMessageDesc desc);

/*
 * Destroy the specified NaClSrpcMessageChannel.
 */
void NaClSrpcMessageChannelDelete(struct NaClSrpcMessageChannel* channel);

/*
 * Messages to be sent or received are described by a message header.
 */
#ifdef __native_client__
/* In untrusted code, NaClSrpcMessageHeader uses untrusted IMC directly. */
typedef struct NaClImcMsgHdr                   NaClSrpcMessageHeader;
#define NACL_SRPC_MESSAGE_HEADER_DESCV         descv
#define NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH   desc_length
#else  /* trusted code */
/* In trusted code, NaClSrpcMessageHeader uses the NRD transfer interface. */
typedef struct NaClImcTypedMsgHdr              NaClSrpcMessageHeader;
#define NACL_SRPC_MESSAGE_HEADER_DESCV         ndescv
#define NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH   ndesc_length
#endif  /* __native_client__ */

/*
 * Sends a message over channel.  On success, returns the number of bytes
 * sent; otherwise, returns a negative NaCl errno.
 */
ssize_t NaClSrpcMessageChannelSend(struct NaClSrpcMessageChannel* channel,
                                   const NaClSrpcMessageHeader* message);

/*
 * Receives a message from channel.  On success, returns the number of bytes
 * received; otherwise, returns a negative NaCl errno.
 */
ssize_t NaClSrpcMessageChannelReceive(struct NaClSrpcMessageChannel* channel,
                                      NaClSrpcMessageHeader* message);

/*
 * Peeks a message from channel.  On success, returns the number of bytes
 * received; otherwise, returns a negative NaCl errno.  The message size that
 * can be peeked is limited to one fragment.
 */
ssize_t NaClSrpcMessageChannelPeek(struct NaClSrpcMessageChannel* channel,
                                   NaClSrpcMessageHeader* header);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_SHARED_SRPC_NACL_SRPC_MESSAGE_H_ */
