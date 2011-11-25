/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


// NaCl inter-module communication primitives.
//
// This file implements common parts of IMC for "unix like systems" (i.e. not
// used on Windows).

// TODO(shiki): Perhaps this file should go into a platform-specific directory
// (posix? unixlike?)  We have a little convention going where mac/linux stuff
// goes in the linux directory and is referenced by the mac build but that's a
// little sloppy.

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <algorithm>

#include "native_client/src/include/atomic_ops.h"

#include "native_client/src/shared/imc/nacl_imc.h"
#include "native_client/src/shared/platform/nacl_check.h"

namespace nacl {

namespace {

const char kNaClTempPrefixVar[] = "NACL_TMPFS_PREFIX";

// The pathname or SHM-namespace prefixes for memory objects created
// by CreateMemoryObject().
const char kShmTempPrefix[] = "/tmp/google-nacl-shm-";
const char kShmOpenPrefix[] = "/google-nacl-shm-";

}  // namespace

bool WouldBlock() {
  return (errno == EAGAIN) ? true : false;
}

int GetLastErrorString(char* buffer, size_t length) {
#if NACL_LINUX
  // Note some Linux distributions provide only GNU version of strerror_r().
  if (buffer == NULL || length == 0) {
    errno = ERANGE;
    return -1;
  }
  char* message = strerror_r(errno, buffer, length);
  if (message != buffer) {
    size_t message_bytes = strlen(message) + 1;
    length = std::min(message_bytes, length);
    memmove(buffer, message, length);
    buffer[length - 1] = '\0';
  }
  return 0;
#else
  return strerror_r(errno, buffer, length);
#endif
}

static Atomic32 memory_object_count = 0;

static int TryShmOrTempOpen(size_t length, const char* prefix, bool use_temp) {
  if (0 == length) {
    return -1;
  }

  char name[PATH_MAX];
  for (;;) {
    snprintf(name, sizeof name, "%s-%u.%u", prefix,
             getpid(),
             static_cast<uint32_t>(AtomicIncrement(&memory_object_count, 1)));
    int m;
    if (use_temp) {
      m = open(name, O_RDWR | O_CREAT | O_EXCL, 0);
    } else {
      m = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0);
    }
    if (0 <= m) {
      if (use_temp) {
        int rc = unlink(name);
        DCHECK(rc == 0);
      } else {
        int rc = shm_unlink(name);
        DCHECK(rc == 0);
      }
      if (ftruncate(m, length) == -1) {
        close(m);
        m = -1;
      }
      return m;
    }
    if (errno != EEXIST) {
      return -1;
    }
    // Retry only if we got EEXIST.
  }
}

static CreateMemoryObjectFunc g_create_memory_object_func = NULL;

void SetCreateMemoryObjectFunc(CreateMemoryObjectFunc func) {
  g_create_memory_object_func = func;
}

Handle CreateMemoryObject(size_t length, bool executable) {
  if (0 == length) {
    return -1;
  }
  int fd;

  if (g_create_memory_object_func != NULL) {
    fd = g_create_memory_object_func(length, executable);
    if (fd >= 0)
      return fd;
  }

  // /dev/shm is not always available on Linux.
  // Sometimes it's mounted as noexec.
  // To handle this case, sel_ldr can take a path
  // to tmpfs from the environment.

#if NACL_LINUX && defined(NACL_ENABLE_TMPFS_REDIRECT_VAR)
  if (NACL_ENABLE_TMPFS_REDIRECT_VAR) {
    const char* prefix = getenv(kNaClTempPrefixVar);
    if (prefix != NULL) {
      fd = TryShmOrTempOpen(length, prefix, true);
      if (fd >= 0) {
        return fd;
      }
    }
  }
#endif

  // On Mac OS X, shm_open() gives us file descriptors that the OS
  // won't mmap() with PROT_EXEC, which is no good for the dynamic
  // code region.  Try open()ing a file in /tmp first, but fall back
  // to using shm_open() if /tmp is not available, which will be the
  // case inside the Chromium sandbox.  This means that dynamic
  // loading will only work with --no-sandbox.
  //
  // TODO(mseaborn): We will probably need to do IPC to acquire SHM FDs
  // inside the Chromium Mac sandbox, as on Linux.

#if NACL_OSX
  // Try /tmp first.  It would be OK to enable this for Linux, but
  // there's no need because shm_open() (which uses /dev/shm rather
  // than /tmp) is fine on Linux.
  fd = TryShmOrTempOpen(length, kShmTempPrefix, true);
  if (fd >= 0)
    return fd;
#endif

  // Try shm_open().
  return TryShmOrTempOpen(length, kShmOpenPrefix, false);
}

void* Map(void* start, size_t length, int prot, int flags,
          Handle memory, off_t offset) {
  static const int kPosixProt[] = {
    PROT_NONE,
    PROT_READ,
    PROT_WRITE,
    PROT_READ | PROT_WRITE,
    PROT_EXEC,
    PROT_READ | PROT_EXEC,
    PROT_WRITE | PROT_EXEC,
    PROT_READ | PROT_WRITE | PROT_EXEC
  };

  int adjusted = 0;
  if (flags & kMapShared) {
    adjusted |= MAP_SHARED;
  }
  if (flags & kMapPrivate) {
    adjusted |= MAP_PRIVATE;
  }
  if (flags & kMapFixed) {
    adjusted |= MAP_FIXED;
  }
  return mmap(start, length, kPosixProt[prot & 7], adjusted, memory, offset);
}

int Unmap(void* start, size_t length) {
  return munmap(start, length);
}

}  // namespace nacl
