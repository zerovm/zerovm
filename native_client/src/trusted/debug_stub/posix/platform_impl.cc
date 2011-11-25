/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>

#include <map>
#include <vector>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/gdb_rsp/abi.h"
#include "native_client/src/trusted/gdb_rsp/util.h"
#include "native_client/src/trusted/port/event.h"
#include "native_client/src/trusted/port/platform.h"

#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"


/*
 * Define the OS specific portions of gdb_utils IPlatform interface.
 */


static port::IEvent* GetLaunchEvent() {
  static port::IEvent* event_ = port::IEvent::Allocate();
  return event_;
}

namespace port {

struct StartInfo_t {
  port::IPlatform::ThreadFunc_t func_;
  void *cookie_;
  volatile uint32_t id_;
};

// Get the OS id of this thread
uint32_t IPlatform::GetCurrentThread() {
  return static_cast<uint32_t>(syscall(SYS_gettid));
}

// Use start stub, to record thread id, and signal launcher
static void *StartFunc(void* cookie) {
  StartInfo_t* info = reinterpret_cast<StartInfo_t*>(cookie);
  info->id_ = (uint32_t) syscall(SYS_gettid);

  printf("Started thread...\n");
  GetLaunchEvent()->Signal();
  info->func_(info->cookie_);

  return NULL;
}

uint32_t IPlatform::CreateThread(ThreadFunc_t func, void* cookie) {
  pthread_t thread;
  StartInfo_t info;

  // Setup the thread information
  info.func_ = func;
  info.cookie_ = cookie;

  printf("Creating thread...\n");

  // Redirect to stub and wait for signal before continuing
  if (pthread_create(&thread, NULL, StartFunc, &info) == 0) {
    GetLaunchEvent()->Wait();
    printf("Found thread...\n");
    return info.id_;
  }

  return 0;
}

void IPlatform::Relinquish(uint32_t msec) {
  usleep(msec * 1000);
}

// In order to read from a pointer that might not be valid, we use the
// trick of getting the kernel to do it on our behalf.
static bool SafeMemoryCopy(void *dest, void *src, size_t len) {
  // The trick only works if we are copying less than the buffer size
  // of a pipe.  For now, return an error on larger sizes.
  // TODO(mseaborn): If we need to copy more, we would have to break
  // it up into smaller parts.
  const size_t kPipeBufferBound = 0x1000;
  if (len > kPipeBufferBound)
    return false;

  bool success = false;
  int pipe_fds[2];
  if (pipe(pipe_fds) != 0)
    return false;
  ssize_t sent = write(pipe_fds[1], src, len);
  if (sent == static_cast<ssize_t>(len)) {
    ssize_t got = read(pipe_fds[0], dest, len);
    if (got == static_cast<ssize_t>(len))
      success = true;
  }
  CHECK(close(pipe_fds[0]) == 0);
  CHECK(close(pipe_fds[1]) == 0);
  return success;
}

bool IPlatform::GetMemory(uint64_t virt, uint32_t len, void *dst) {
  return SafeMemoryCopy(dst, reinterpret_cast<void*>(virt), len);
}

bool IPlatform::SetMemory(uint64_t virt, uint32_t len, void *src) {
  uintptr_t page_mask = NACL_PAGESIZE - 1;
  uintptr_t page = virt & ~page_mask;
  uintptr_t mapping_size = ((virt + len + page_mask) & ~page_mask) - page;
  if (mprotect(reinterpret_cast<void*>(page), mapping_size,
               PROT_READ | PROT_WRITE) != 0) {
    return false;
  }
  bool succeeded = SafeMemoryCopy(reinterpret_cast<void*>(virt), src, len);
  // TODO(mseaborn): We assume here that SetMemory() is being used to
  // set or remove a breakpoint in the code area, so that PROT_READ |
  // PROT_EXEC are the correct flags to restore the mapping to.
  // The earlier mprotect() does not tell us what the original flags
  // were.  To find this out we could either:
  //  * read /proc/self/maps (not available inside outer sandbox); or
  //  * use service_runtime's own mapping tables.
  // Alternatively, we could modify code the same way nacl_text.c does.
  if (mprotect(reinterpret_cast<void*>(page), mapping_size,
               PROT_READ | PROT_EXEC) != 0) {
    return false;
  }
  return succeeded;
}

}  // End of port namespace

