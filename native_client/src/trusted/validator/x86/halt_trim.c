/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/validator/x86/halt_trim.h"

#include <stdio.h>
#include "native_client/src/trusted/validator/x86/ncinstbuffer.h"

/* Safety buffer size of halts we must keep, so that we guarantee
 * that we don't trim the last legal instruction in the
 * code segment. Note: we add 1 to the maximum instruction length just
 * to be safe.
 */
static const NaClMemorySize kMinHaltKeepLength = MAX_INST_LENGTH + 1;

/* x86 HALT opcode */
static const uint8_t kNaClHalt = 0xf4;

typedef struct NaClHaltTrimData {
  /* The base address of the memory segment. */
  uint8_t *mbase;
  /* The base virtual address associated with the beginning
   * address of the memory segment.
   */
  NaClPcAddress vbase;
  /* The size of the memory segment. Updated if trimming occurred. to
   * the new value.
   */
  NaClMemorySize sz;
  /* The byte (modulo) alignment to use. */
  uint8_t alignment;
  /* The new limit to use if trimming occurred. */
  NaClPcAddress vlimit;
} NaClHaltTrimData;

static NaClMemorySize NCHaltTrimSize(uint8_t* mbase, NaClPcAddress vbase,
                                     NaClMemorySize sz, uint8_t alignment) {
  NaClMemorySize i;
  NaClMemorySize num_halts;
  for (i = sz - 1; i > 0; --i) {
    if (kNaClHalt != mbase[i]) break;
  }
  num_halts = sz - (i + 1);
  if (num_halts > kMinHaltKeepLength) {
    /* May be able to trim off trailing halts. */
    NaClPcAddress end_pc;
    NaClPcAddress block_overflow;
    end_pc = vbase + (sz - num_halts) + kMinHaltKeepLength;
    block_overflow = end_pc % alignment;
    if (block_overflow) {
      /* Add rounding to nearest block alignment if possible. */
      NaClPcAddress new_pc = end_pc + alignment - block_overflow;
      NaClPcAddress vlimit = vbase + sz;
      if (new_pc < vlimit) {
        end_pc = new_pc;
      } else {
        /* Can't add alignment, exceeds memory limit! Hence,
         * give up.
         */
        return sz;
      }
    }
    sz = end_pc - vbase;
  }
  return sz;
}

void NCHaltTrimSegment(uint8_t *mbase,
                       NaClPcAddress vbase,
                       uint8_t alignment,
                       NaClMemorySize *sz,
                       NaClPcAddress *vlimit) {
  if (*vlimit == (vbase + *sz)) {
    /* Code segment is trailling code segment. Safe to remove halts. */
    NaClMemorySize new_size = NCHaltTrimSize(mbase, vbase, *sz, alignment);
    if (new_size != *sz) {
      NaClMemorySize diff = *sz - new_size;
      *sz -= diff;
      *vlimit -= diff;
    }
  }
}
