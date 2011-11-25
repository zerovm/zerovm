/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */


// This module provides interfaces for an IO stream.  The stream is
// expected to throw a std::exception if the stream is terminated on
// either side.
#ifndef NATIVE_CLIENT_PORT_TRANSPORT_H_
#define NATIVE_CLIENT_PORT_TRANSPORT_H_

#include "native_client/src/trusted/port/std_types.h"

namespace port {

class ITransport {
 public:
  // Read from this transport, return a negative value if there is an error
  // otherwise return the number of bytes actually read.
  virtual int32_t Read(void *ptr, int32_t len) = 0;

  // Write to this transport, return a negative value if there is an error
  // otherwise return the number of bytes actually written.
  virtual int32_t Write(const void *ptr, int32_t len)  = 0;

  // Return true once read will not block or false after ms milliseconds.
  virtual bool ReadWaitWithTimeout(uint32_t ms) = 0;

  // Disconnect the transport, R/W and Select will now throw an exception
  virtual void Disconnect() = 0;

  // Attempt to connect to, or accept connection at the specified address
  static ITransport *Connect(const char *addr);
  static ITransport *Accept(const char *addr);
  static void Free(ITransport *transport);

 protected:
  virtual ~ITransport() {}  // Prevent delete of base pointer
};


}  // namespace port

#endif  // NATIVE_CLIENT_PORT_TRANSPORT_H_

