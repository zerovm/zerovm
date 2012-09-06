/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// NaCl inter-module communication primitives.

#include <unistd.h>
#include "src/imc/nacl_imc.h"

namespace nacl {

int Close(Handle handle) {
  return close(handle);
}

}  // namespace nacl
