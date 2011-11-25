/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */


// This module defines the interface for interacting with platform specific
// threads  .  This API provides a mechanism to query for a thread, by using
// the acquire method with the ID of a pre-existing thread.   The register
// accessors are expected to return false if the thread is not in a state
// where the registers can be accessed, such RUNNING or SYSCALL. This API
// will throw:
//   std::exception - if a unexpected OS Error is encountered.
//   std::out_of_range - if the register index is out of range.

#ifndef NATIVE_CLIENT_PORT_THREAD_H_
#define NATIVE_CLIENT_PORT_THREAD_H_ 1

#include <stdlib.h>
#include <map>

#include "native_client/src/trusted/port/std_types.h"

namespace port {

class IThread {
 public:
  enum State {
    DEAD     =-1,  // The thread has exited or been killed
    RUNNING  = 0,  // The thread is currently running
    SUSPENDED= 1,  // The thread has been suspended
    SIGNALED = 2,  // The thread is signaled
    SYSCALL  = 3   // In a sys call, it's registers can not be modified.
  };

  typedef void (*CatchFunc_t)(uint32_t id, int8_t sig, void *cookie);
  typedef std::map<uint32_t, IThread*> ThreadMap_t;

  virtual uint32_t GetId() = 0;
  virtual State GetState() = 0;

  virtual bool SetStep(bool on) = 0;

  virtual bool GetRegister(uint32_t index, void *dst, uint32_t len) = 0;
  virtual bool SetRegister(uint32_t index, void *src, uint32_t len) = 0;

  virtual bool Suspend() = 0;
  virtual bool Resume() = 0;

  virtual void *GetContext() = 0;

  static IThread *Acquire(uint32_t id, bool create = true);
  static void Release(IThread *thread);
  static void SetExceptionCatch(CatchFunc_t func, void *cookie);

 protected:
  virtual ~IThread() {}  // Prevent delete of base pointer
};

}  // namespace port

#endif  // PORT_THREAD_H_

