/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/shared/platform/nacl_sync_checked.h"

/*
 * Define the common gdb_utils IEvent interface to use the NaCl version.
 * IEvent only suports single trigger Signal API.
 */

#include "native_client/src/trusted/port/event.h"

namespace port {

class Event : public IEvent {
 public:
  Event() : signaled_(false) {
    NaClXMutexCtor(&mutex_);
    NaClXCondVarCtor(&cond_);
  }
  ~Event() {
    NaClCondVarDtor(&cond_);
    NaClMutexDtor(&mutex_);
  }

  void Wait() {
    /* Start the wait owning the lock */
    NaClXMutexLock(&mutex_);

    /* We can skip this if already signaled */
    while (!signaled_) {
      /* Otherwise, try and wait, which release the lock */
      NaClXCondVarWait(&cond_, &mutex_);

      /* We exit the wait owning the lock again. */
    };

    /* Clear the signal then unlock. */
    signaled_ = false;
    NaClXMutexUnlock(&mutex_);
  }

  void Signal() {
    signaled_ = true;
    NaClXCondVarSignal(&cond_);
  }

 public:
  volatile bool signaled_;
  NaClMutex mutex_;
  NaClCondVar cond_;
};

IEvent* IEvent::Allocate() {
  return new Event;
}

void IEvent::Free(IEvent *ievent) {
  delete static_cast<Event*>(ievent);
}

}  // End of port namespace
