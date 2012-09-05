/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */


// NaCl inter-module communication primitives.

#include "src/imc/nacl_imc.h"
#include "src/imc/nacl_imc_c.h"

//
// "C" bindings
//

// ###
//int NaClSocketPair(NaClHandle pair[2]) {
//  return nacl::SocketPair(pair);
//}

int NaClClose(NaClHandle handle) {
  return nacl::Close(handle);
}

// ###
//int NaClWouldBlock() {
//  return nacl::WouldBlock();
//}

// ###
//int NaClSendDatagram(NaClHandle socket, const NaClMessageHeader* message,
//                    int flags) {
//  return nacl::SendDatagram(
//      socket,
//      reinterpret_cast<const nacl::MessageHeader*>(message),
//      flags);
//}

// ###
//int NaClReceiveDatagram(NaClHandle socket, NaClMessageHeader* message,
//                       int flags) {
//  return nacl::ReceiveDatagram(socket,
//                               reinterpret_cast<nacl::MessageHeader*>(message),
//                               flags);
//}

NaClHandle NaClCreateMemoryObject(size_t length, int executable) {
  return nacl::CreateMemoryObject(length, executable ? true : false);
}

void* NaClMap(void* start, size_t length, int prot, int flags,
              NaClHandle memory, off_t offset) {
  return nacl::Map(start, length, prot, flags, memory, offset);
}

int NaClUnmap(void* start, size_t length) {
  return nacl::Unmap(start, length);
}
