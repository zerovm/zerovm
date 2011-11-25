/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Defines the notion of a code segment.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NC_SEGMENT_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NC_SEGMENT_H_

#include "native_client/src/trusted/validator/types_memory_model.h"

EXTERN_C_BEGIN

/* Model of a code segment. */
typedef struct NaClSegment {
  /* Points the the beginning of the sequence of bytes in the code segment. */
  uint8_t* mbase;
  /* Defines the virtual pc value associated with the beginning
   * of the code segment.
   */
  NaClPcAddress vbase;
  /* Defines the maximum+1 (virtual) pc value. Used to define
   * when the end of the segment is reached. Corresponds to
   * vbase + size;
   */
  NaClPcAddress vlimit;
  /* The number of bytes in the code segment. */
  NaClMemorySize size;
} NaClSegment;

/* Initializes the given code segment with the given values. */
void NaClSegmentInitialize(
    uint8_t* mbase,
    NaClPcAddress vbase,
    NaClMemorySize size,
    NaClSegment* segment);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NC_SEGMENT_H_ */
