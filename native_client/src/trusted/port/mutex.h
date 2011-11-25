/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */


// This module defines the interface for using platform specific mutexes.
// It is expected that the Allocation function will return NULL on any
// failure instead of throwing an exception.  This module is expected
// to throw a std::exception when an unexpected OS error is encoutered.
//
// The mutex is owned by a single thread at a time.  The thread that
// owns the mutex is free to call Lock multiple times, however it must
// call Unlock the same number of times to release the lock.
#ifndef NATIVE_CLIENT_PORT_MUTEX_H_
#define NATIVE_CLIENT_PORT_MUTEX_H_ 1

#include <assert.h>
#include <stddef.h>

namespace port {

class IMutex {
 public:
  virtual void Lock() = 0;       // Block until the mutex is taken
  virtual void Unlock() = 0;     // Unlock the mutext
  virtual bool Try() = 0;        // Try to lock, but return immediately

  static IMutex *Allocate();      // Allocate a mutex
  static void Free(IMutex *mtx);  // Free a mutex

 protected:
  virtual ~IMutex() {}            // Prevent delete of base pointer
};


// MutexLock
//   A MutexLock object will lock on construction and automatically
// unlock on destruction of the object as the object goes out of scope.
class MutexLock {
 public:
  explicit MutexLock(IMutex *mutex) : mutex_(mutex) {
    assert(NULL != mutex_);
    mutex_->Lock();
  }
  ~MutexLock() {
    mutex_->Unlock();
  }

 private:
  IMutex *mutex_;
  MutexLock(const MutexLock&);
  MutexLock &operator=(const MutexLock&);
};


}  // namespace port

#endif  // NATIVE_CLIENT_PORT_MUTEX_H_

