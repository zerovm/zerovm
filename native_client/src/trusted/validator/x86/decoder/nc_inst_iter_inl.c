/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* nc_inst_iter_inl.c - Holds inline routines for commonly used (simple)
 * functions in nc_inst_iter.h. Used to speed up code. Inline routines
 * correspond to the following functions in nc_inst_iter.h, but with
 * an 'Inline' suffix:
 *
 *    NaClInstIterGetState
 *    NaClInstIterGetLookbackState
 *    NaClInstIterHasLookbackState
 *    NaClInstIterHasNext
 *    NaClInstIterAdvance
 *    NaClInstIterGetInstMemory
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_ITER_INL_h_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_ITER_INL_h_

#include <assert.h>

#include "native_client/src/trusted/validator/x86/decoder/nc_inst_iter.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_state.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_state_internal.h"
#include "native_client/src/trusted/validator/x86/nc_segment.h"

/* Default handler for errors while running instruction iterator.
 * Should only be called when caller has incorrectly called a
 * method.
 */
void NaClInstIterFatal(const char* error_message);

static INLINE NaClInstState* NaClInstIterGetUndecodedStateInline(
    NaClInstIter* iter) {
  return &iter->buffer[iter->buffer_index];
}

static INLINE NaClInstState* NaClInstIterGetStateInline(NaClInstIter* iter) {
  NaClInstState* state = NaClInstIterGetUndecodedStateInline(iter);
  if (NULL == state->inst) {
    NaClDecodeInst(iter, state);
  }
  return state;
}

static INLINE NaClInstState* NaClInstIterGetLookbackStateInline(
    NaClInstIter* iter, size_t distance) {
  NaClInstState* state;
  assert(distance < iter->buffer_size);
  assert(distance <= iter->inst_count);
  state = &iter->buffer[((iter->buffer_index + iter->buffer_size) - distance)
                        % iter->buffer_size];
  if (NULL == state->inst) {
    NaClDecodeInst(iter, state);
  }
  return state;
}

static INLINE Bool NaClInstIterHasLookbackStateInline(
    NaClInstIter* iter, size_t distance) {
  return distance < iter->buffer_size && distance <= iter->inst_count;
}

static INLINE Bool NaClInstIterHasNextInline(NaClInstIter* iter) {
  /*
  DEBUG(NaClLog(LOG_INFO, "iter has next index %"NACL_PRIxNaClMemorySize
                " < %"NACL_PRIxNaClMemorySize"\n",
                iter->index, iter->segment->size));
  */
  return iter->index < iter->segment->size;
}

static INLINE void NaClInstIterAdvanceInline(NaClInstIter* iter) {
  if (iter->index >= iter->segment->size) {
    NaClInstIterFatal("NaClInstIterAdvance with no next element.");
  }
  NaClInstIterGetStateInline(iter);
  iter->index += iter->memory.read_length;
  ++iter->inst_count;
  iter->buffer_index = (iter->buffer_index + 1) % iter->buffer_size;
  /*
  DEBUG(
      NaClLog(LOG_INFO,
              "iter advance: index %"NACL_PRIxNaClMemorySize", "
              "buffer index %"NACL_PRIuS"\n",
              iter->index, iter->buffer_index));
  */
  iter->buffer[iter->buffer_index].inst = NULL;
}

static INLINE uint8_t* NaClInstIterGetInstMemoryInline(NaClInstIter* iter) {
  if (iter->index >= iter->segment->size) {
    NaClInstIterFatal("NaClInstIterGetInstMemory with no next element.");
  }
  return iter->segment->mbase + iter->index;
}

#endif /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_ITER_INL_h_ */
