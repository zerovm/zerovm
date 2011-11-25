/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include <pthread.h>

#include "native_client/src/trusted/port/mutex.h"

/*
 * Unfortunately NaClSync does not have the correct recursive
 * property so we need to use our own version.
 */

namespace port {

class Mutex : public IMutex {
 public:
  Mutex() {
    pthread_mutexattr_t attr;

    // Create a recursive mutex, so we can reenter on the same thread
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex_, &attr);
    pthread_mutexattr_destroy(&attr);
  }

  ~Mutex() {
    pthread_mutex_destroy(&mutex_);
  }

  virtual void Lock() {
    pthread_mutex_lock(&mutex_);
  }

  virtual bool Try() {
    return pthread_mutex_trylock(&mutex_) == 0;
  }

  virtual void Unlock() {
    pthread_mutex_unlock(&mutex_);
  }

 public:
  pthread_mutex_t mutex_;
};

IMutex* IMutex::Allocate() {
  return new Mutex;
}

void IMutex::Free(IMutex *mutex) {
  delete static_cast<Mutex*>(mutex);
}

}  // namespace port
