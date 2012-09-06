/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


// NaCl inter-module communication primitives.

#include "src/imc/nacl_imc.h"
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "src/platform/nacl_log.h"

namespace nacl {

int Close(Handle handle) {
  return close(handle);
}

}  // namespace nacl
