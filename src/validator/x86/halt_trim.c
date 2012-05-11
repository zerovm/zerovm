/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/validator/x86/halt_trim.h"

#include <stdio.h>
#include "src/validator/x86/ncinstbuffer.h"

/* Safety buffer size of halts we must keep, so that we guarantee
 * that we don't trim the last legal instruction in the
 * code segment. Note: we add 1 to the maximum instruction length just
 * to be safe.
 */
static const NaClMemorySize kMinHaltKeepLength = MAX_INST_LENGTH + 1;

/* x86 HALT opcode */
static const uint8_t kNaClHalt = 0xf4;

NaClMemorySize NCHaltTrimSize(uint8_t *mbase, NaClMemorySize sz,
                              uint8_t alignment) {
  NaClMemorySize i;
  NaClMemorySize num_halts;
  for (i = sz - 1; i > 0; --i) {
    if (kNaClHalt != mbase[i]) break;
  }
  num_halts = sz - (i + 1);
  if (num_halts > kMinHaltKeepLength) {
    /* May be able to trim off trailing halts. */
    NaClMemorySize new_size;
    NaClMemorySize block_overflow;
    new_size = (sz - num_halts) + kMinHaltKeepLength;
    /* Round to nearest block alignment. */
    block_overflow = new_size % alignment;
    if (block_overflow) {
      new_size = new_size - block_overflow + alignment;
    }
    /* Never increase the segment size. */
    if (new_size < sz) {
      return new_size;
    }
  }
  /* No trim performed. */
  return sz;
}
