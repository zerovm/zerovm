/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Defines an instruction (decoder) iterator that processes code segments.
 */

#include "native_client/src/trusted/validator/x86/decoder/nc_inst_iter.h"

#include <stdio.h>
#include <stdlib.h>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_state.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_state_internal.h"
#include "native_client/src/trusted/validator/x86/decoder/nc_inst_trans.h"
#include "native_client/src/trusted/validator/x86/decoder/ncop_exps.h"

/* To turn on debugging of instruction decoding, change value of
 * DEBUGGING to 1.
 */
#define DEBUGGING 0

#include "native_client/src/shared/utils/debugging.h"

#include "native_client/src/trusted/validator/x86/decoder/nc_inst_iter_inl.c"

static void NaClInstIterLogError(const char* error_message) {
  NaClLog(LOG_ERROR, "*ERROR* %s\n", error_message);
}

void NaClInstIterFatal(const char* error_message) {
  NaClInstIterLogError(error_message);
  exit(1);
}

/* Default handler for errors found while parsing the memory segment.*/
static void NaClInstIterReportRemainingMemoryError(
    NCRemainingMemoryError error,
    struct NCRemainingMemory* memory) {
  NaClInstIterLogError(NCRemainingMemoryErrorMessage(error));
}

NaClInstIter* NaClInstIterCreateWithLookback(
    const struct NaClDecodeTables* decoder_tables,
    NaClSegment* segment,
    size_t lookback_size) {
  NaClInstIter* iter;
  /* Guarantee we don't wrap around while computing buffer index updates. */
  assert(((lookback_size + 1) * 2 + 1) > lookback_size);
  iter = (NaClInstIter*) malloc(sizeof(NaClInstIter));
  if (NULL != iter) {
    iter->segment = segment;
    NCRemainingMemoryInit(segment->mbase, segment->size, &iter->memory);
    iter->memory.error_fn = NaClInstIterReportRemainingMemoryError;
    iter->index = 0;
    iter->inst_count = 0;
    iter->buffer_size = lookback_size + 1;
    iter->buffer_index = 0;
    iter->buffer = (NaClInstState*)
        calloc(iter->buffer_size, sizeof iter->buffer[0]);
    if (NULL == iter->buffer) {
      free(iter);
      iter = NULL;
    } else {
      size_t i;
      for (i = 0; i < iter->buffer_size; ++i) {
        iter->buffer[i].inst = NULL;
        NCInstBytesInitMemory(&iter->buffer[i].bytes, &iter->memory);
      }
    }
  }
  iter->decoder_tables = (struct NaClDecodeTables*) decoder_tables;
  return iter;
}

NaClInstIter* NaClInstIterCreate(
    const struct NaClDecodeTables* decoder_tables,
    NaClSegment* segment) {
  return NaClInstIterCreateWithLookback(decoder_tables, segment, 0);
}

void NaClInstIterDestroy(NaClInstIter* iter) {
  if (NULL != iter) {
    free(iter->buffer);
    free(iter);
  }
}

NaClInstState* NaClInstIterGetUndecodedState(NaClInstIter* iter) {
  return NaClInstIterGetUndecodedStateInline(iter);
}

NaClInstState* NaClInstIterGetState(NaClInstIter* iter) {
  return NaClInstIterGetStateInline(iter);
}

Bool NaClInstIterHasLookbackState(NaClInstIter* iter, size_t distance) {
  return NaClInstIterHasLookbackStateInline(iter, distance);
}

NaClInstState* NaClInstIterGetLookbackState(NaClInstIter* iter,
                                            size_t distance) {
  return NaClInstIterGetLookbackStateInline(iter, distance);
}

Bool NaClInstIterHasNext(NaClInstIter* iter) {
  return NaClInstIterHasNextInline(iter);
}

void NaClInstIterAdvance(NaClInstIter* iter) {
  NaClInstIterAdvanceInline(iter);
}

uint8_t* NaClInstIterGetInstMemory(NaClInstIter* iter) {
  return NaClInstIterGetInstMemoryInline(iter);
}
