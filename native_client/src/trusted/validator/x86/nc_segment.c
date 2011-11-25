/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Defines the notion of a code segment.
 */

#include "native_client/src/trusted/validator/x86/nc_segment.h"

void NaClSegmentInitialize(
    uint8_t* mbase,
    NaClPcAddress vbase,
    NaClMemorySize size,
    NaClSegment* segment) {
  segment->mbase = mbase;
  segment->vbase = vbase;
  segment->vlimit = vbase + size;
  segment->size = size;
}
