/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */


// NaCl inter-module communication primitives.

// TODO(robertm): stdio.h is included for NULL only - find a better way
#include <stdio.h>

#include "native_client/src/include/portability.h"

#include "native_client/src/shared/imc/nacl_imc.h"
#include "native_client/src/shared/platform/nacl_log.h"

namespace nacl {

static int InitHeader(IOVec *vec,
                         MessageHeader *header,
                         const void* buffer,
                         size_t length) {
  vec->base = const_cast<void*>(buffer);
  if (length > UINT32_MAX) {
    return -1;
  }
  vec->length = static_cast<unsigned int>(length);

  header->iov = vec;
  header->iov_length = 1;
  header->handles = NULL;
  header->handle_count = 0;
  header->flags = 0;

  return 0;
}

int Send(Handle socket, const void* buffer, size_t length, int flags) {
  MessageHeader header;
  IOVec vec;
  int retval;

  retval = InitHeader(&vec, &header, buffer, length);
  if (retval) {
    return retval;
  }

  return SendDatagram(socket, &header, flags);
}

int Receive(Handle socket, void* buffer, size_t length, int flags) {
  MessageHeader header;
  IOVec vec;
  int retval;

  retval = InitHeader(&vec, &header, buffer, length);
  if (retval) {
    return retval;
  }

  return ReceiveDatagram(socket, &header, flags);
}

bool MessageSizeIsValid(const MessageHeader *message) {
  size_t cur_bytes = 0;
  static size_t const kMax = static_cast<size_t>(~static_cast<uint32_t>(0));
  /* we assume that sizeof(uint32_t) <= sizeof(size_t) */

  for (size_t ix = 0; ix < message->iov_length; ++ix) {
    if (kMax - cur_bytes < message->iov[ix].length) {
      return false;
    }
    cur_bytes += message->iov[ix].length;  /* no overflow is possible */
  }
  return true;
}

}  // namespace nacl
