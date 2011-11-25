/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include <stdexcept>
#include <string.h>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/gdb_rsp/abi.h"
#include "native_client/src/trusted/port/mutex.h"
#include "native_client/src/trusted/port/platform.h"
#include "native_client/src/trusted/port/thread.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"

/*
 * Define the OS specific portions of gdb_utils IThread interface.
 */

namespace {

const int kX86TrapFlag = 1 << 8;

}  // namespace

namespace port {

static IThread::CatchFunc_t s_CatchFunc = NULL;
static void* s_CatchCookie = NULL;

static IMutex* ThreadGetLock() {
  static IMutex* mutex_ = IMutex::Allocate();
  return mutex_;
}

static IThread::ThreadMap_t *ThreadGetMap() {
  static IThread::ThreadMap_t* map_ = new IThread::ThreadMap_t;
  return map_;
}

class Thread : public IThread {
 public:
  explicit Thread(uint32_t id) : ref_(1), id_(id), state_(DEAD) {}
  ~Thread() {}

  uint32_t GetId() {
    return id_;
  }

  State GetState() {
    return state_;
  }

  virtual bool Suspend() {
    // TODO(mseaborn): Implement this.  Unlike with Windows'
    // SuspendThread(), a thread cannot be suspended from another
    // thread in Unix.  We would have to send the thread a signal to
    // ask it to suspend.
    return false;
  }

  virtual bool Resume() {
    // TODO(mseaborn): Implement this.
    return false;
  }

  // TODO(mseaborn): Similar logic is duplicated in the Windows version.
  virtual bool SetStep(bool on) {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
    if (on) {
      context_.flags |= kX86TrapFlag;
    } else {
      context_.flags &= ~kX86TrapFlag;
    }
    return true;
#else
    // TODO(mseaborn): Implement for ARM.
    UNREFERENCED_PARAMETER(on);
    return false;
#endif
  }

  virtual bool GetRegister(uint32_t index, void *dst, uint32_t len) {
    const gdb_rsp::Abi *abi = gdb_rsp::Abi::Get();
    const gdb_rsp::Abi::RegDef *reg = abi->GetRegisterDef(index);
    memcpy(dst, (char *) &context_ + reg->offset_, len);
    return false;
  }

  virtual bool SetRegister(uint32_t index, void* src, uint32_t len) {
    const gdb_rsp::Abi *abi = gdb_rsp::Abi::Get();
    const gdb_rsp::Abi::RegDef *reg = abi->GetRegisterDef(index);
    memcpy((char *) &context_ + reg->offset_, src, len);
    return false;
  }

  // This is not used and could be removed.
  virtual void* GetContext() { return NULL; }

  static enum NaClSignalResult SignalHandler(int signal, void *ucontext) {
    struct NaClSignalContext context;
    NaClSignalContextFromHandler(&context, ucontext);
    if (NaClSignalContextIsUntrusted(&context)) {
      uint32_t thread_id = IPlatform::GetCurrentThread();
      Thread* thread = static_cast<Thread*>(Acquire(thread_id));
      State old_state = thread->state_;
      thread->state_ = SIGNALED;
      thread->context_ = context;

      if (s_CatchFunc != NULL)
        s_CatchFunc(thread_id, signal, s_CatchCookie);

      NaClSignalContextToHandler(ucontext, &thread->context_);
      thread->state_ = old_state;
      Release(thread);
      return NACL_SIGNAL_RETURN;
    } else {
      // Do not attempt to debug crashes in trusted code.
      return NACL_SIGNAL_SEARCH;
    }
  }

 private:
  uint32_t ref_;
  uint32_t id_;
  State  state_;
  struct NaClSignalContext context_;

  friend class IThread;
};

// TODO(mseaborn): This is duplicated in the Windows version.
IThread* IThread::Acquire(uint32_t id, bool create) {
  MutexLock lock(ThreadGetLock());
  Thread* thread;
  ThreadMap_t &map = *ThreadGetMap();

  // Check if we have that thread
  if (map.count(id)) {
    thread = static_cast<Thread*>(map[id]);
    thread->ref_++;
    return thread;
  }

  // If not, can we create it?
  if (create) {
    // If not add it to the map
    thread = new Thread(id);
    map[id] = thread;
    return thread;
  }

  return NULL;
}

// TODO(mseaborn): This is duplicated in the Windows version.
void IThread::Release(IThread *ithread) {
  MutexLock lock(ThreadGetLock());
  Thread* thread = static_cast<Thread*>(ithread);
  thread->ref_--;

  if (thread->ref_ == 0) {
    ThreadGetMap()->erase(thread->id_);
    delete static_cast<IThread*>(thread);
  }
}

void IThread::SetExceptionCatch(IThread::CatchFunc_t func, void *cookie) {
  NaClSignalHandlerAdd(Thread::SignalHandler);
  s_CatchFunc = func;
  s_CatchCookie = cookie;
}


}  // End of port namespace

