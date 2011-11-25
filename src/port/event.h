/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */


// This module defines the interface for using platform specific events.
// It is expected that the Allocation function will return NULL on any
// failure instead of throwing an exception.  This module is expected
// to throw a std::exception when an unexpected OS error is encoutered.
#ifndef NATIVE_CLIENT_PORT_EVENT_H_
#define NATIVE_CLIENT_PORT_EVENT_H_ 1

namespace port {

class IEvent {
 public:
  virtual void Signal() = 0;    // Free one waiting thread
  virtual void Wait() = 0;      // Suspend this thread waiting for signal

  static IEvent *Allocate();    // Allocate an IEvent
  static void Free(IEvent *e);  // Free the IEvent

 protected:
  virtual ~IEvent() {}          // Prevent delete of base pointer
};


}  // namespace port

#endif  // NATIVE_CLIENT_PORT_EVENT_H_

