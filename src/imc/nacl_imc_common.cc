/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */


// NaCl inter-module communication primitives.

// TODO(robertm): stdio.h is included for NULL only - find a better way
#include "src/imc/nacl_imc.h"

namespace nacl {

//bool MessageSizeIsValid(const MessageHeader *message) {
//  size_t cur_bytes = 0;
//  static size_t const kMax = static_cast<size_t>(~static_cast<uint32_t>(0));
//  /* we assume that sizeof(uint32_t) <= sizeof(size_t) */
//
//  for (size_t ix = 0; ix < message->iov_length; ++ix) {
//    if (kMax - cur_bytes < message->iov[ix].length) {
//      return false;
//    }
//    cur_bytes += message->iov[ix].length;  /* no overflow is possible */
//  }
//  return true;
//}

}  // namespace nacl
