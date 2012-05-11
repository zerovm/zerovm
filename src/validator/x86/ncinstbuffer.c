/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>

#include "src/platform/nacl_check.h"
#include "src/validator/x86/ncinstbuffer.h"

/* To turn on debugging of instruction decoding, change value of
 * DEBUGGING to 1.
 */
#define DEBUGGING 0

#include "src/utils/debugging.h"

#include "src/validator/x86/ncinstbuffer_inl.c"

void NCRemainingMemoryAdvance(NCRemainingMemory* memory) {
  NCRemainingMemoryAdvanceInline(memory);
}

void NCRemainingMemoryReset(NCRemainingMemory* memory) {
  NCRemainingMemoryResetInline(memory);
}

const char* NCRemainingMemoryErrorMessage(NCRemainingMemoryError error) {
  switch (error) {
    case NCRemainingMemoryOverflow:
      return "Read past end of memory segment occurred.";
    case NCInstBufferOverflow:
      return "Internal error: instruction buffer overflow.";
    case NCUnknownMemoryError:
    default:
      return "Unknown memory error occurred.";
  }
}

void NCRemainingMemoryReportError(NCRemainingMemoryError error,
                                  NCRemainingMemory* memory) {
  fprintf(stdout, "%s\n", NCRemainingMemoryErrorMessage(error));
}

void NCRemainingMemoryInit(uint8_t* memory_base, NaClMemorySize size,
                           NCRemainingMemory* memory) {
  memory->mpc = memory_base;
  memory->cur_pos = memory->mpc;
  memory->mlimit = memory_base + size;
  memory->next_byte = NCRemainingMemoryPeekInline(memory);
  memory->error_fn = NCRemainingMemoryReportError;
  memory->error_fn_state = NULL;
  NCRemainingMemoryAdvanceInline(memory);
}

uint8_t NCRemainingMemoryLookahead(NCRemainingMemory* memory, ssize_t n) {
  return NCRemainingMemoryLookaheadInline(memory, n);
}

uint8_t NCRemainingMemoryRead(NCRemainingMemory* memory) {
  return NCRemainingMemoryReadInline(memory);
}

void NCInstBytesInitMemory(NCInstBytes* bytes, NCRemainingMemory* memory) {
#if NCBUF_CLEAR_CACHE
  int i;
  for (i = 0; i < MAX_INST_LENGTH; ++i) {
    bytes->byte[i] = 0;
  }
#endif
  bytes->memory = memory;
  bytes->length = 0;
}

void NCInstBytesReset(NCInstBytes* buffer) {
  NCInstBytesResetInline(buffer);
}

void NCInstBytesInit(NCInstBytes* buffer) {
  NCInstBytesInitInline(buffer);
}

uint8_t NCInstBytesPeek(NCInstBytes* bytes, ssize_t n) {
  return NCInstBytesPeekInline(bytes, n);
}

uint8_t NCInstByte(NCInstBytes* bytes, ssize_t n) {
  return NCInstByteInline(bytes, n);
}

uint8_t NCInstBytesRead(NCInstBytes* bytes) {
  return NCInstBytesReadInline(bytes);
}

void NCInstBytesReadBytes(ssize_t n, NCInstBytes* bytes) {
  NCInstBytesReadBytesInline(n, bytes);
}

#if NCBUF_CLEAR_CACHE
#define NCBUF_BYTES_LENGTH(bytes) MAX_INST_LENGTH
#else
#define NCBUF_BYTES_LENGTH(bytes) (bytes)->length
#endif

static INLINE void NCInstBytesPtrInitPos(
    NCInstBytesPtr* ptr, const NCInstBytes* bytes, int pos) {
  ptr->bytes = bytes;
  if (pos <= NCBUF_BYTES_LENGTH(bytes)) {
    ptr->pos = (uint8_t) pos;
  } else {
    bytes->memory->error_fn(NCInstBufferOverflow, bytes->memory);
    ptr->pos = bytes->length;
  }
}

void NCInstBytesPtrInit(NCInstBytesPtr* ptr, const NCInstBytes* bytes) {
  NCInstBytesPtrInitPos(ptr, bytes, 0);
}

void NCInstBytesPtrInitInc(NCInstBytesPtr* ptr, const NCInstBytesPtr* base,
                           int pos) {
  NCInstBytesPtrInitPos(ptr, base->bytes, base->pos + pos);
}

uint8_t NCInstBytesPos(const NCInstBytesPtr* ptr) {
  return ptr->pos;
}

uint8_t NCInstBytesByte(const NCInstBytesPtr* ptr, int n) {
  return NCInstBytesByteInline(ptr, n);
}

int32_t NCInstBytesInt32(const NCInstBytesPtr* ptr, int num_bytes) {
  switch (num_bytes) {
    case 1:
      return (int8_t) NCInstBytesByteInline(ptr, 0);
    case 2:
      return (int16_t) (NCInstBytesByteInline(ptr, 0) +
                        (NCInstBytesByteInline(ptr, 1) << 8));
    case 3:
      /* Note: Handle special case of Iw, Ib in 32 bit validator. */
      return (int32_t) (NCInstBytesByteInline(ptr, 0) +
                        (NCInstBytesByteInline(ptr, 1) << 8) +
                        (NCInstBytesByteInline(ptr, 2) << 16));
    case 4:
      return (int32_t) (NCInstBytesByteInline(ptr, 0) +
                        (NCInstBytesByteInline(ptr, 1) << 8) +
                        (NCInstBytesByteInline(ptr, 2) << 16) +
                        (NCInstBytesByteInline(ptr, 3) << 24));
    default:
      CHECK(0); /* Fail -- should not happen. */
      return -1;
  }
}

int64_t NCInstBytesInt64(const NCInstBytesPtr* ptr, int num_bytes) {
  switch (num_bytes) {
    case 1:
    case 2:
    case 3: /* Handle special case of Iw, Ib in 32 bit validator. */
    case 4:
      return (int64_t) NCInstBytesInt32(ptr, num_bytes);
    case 6: {
      /* Handle special case of 48-bit pointers in 32 bit validator. */
      NCInstBytesPtr ptr_plus_2;
      NCInstBytesPtrInitInc(&ptr_plus_2, ptr, 2);
      return ((int64_t) (NCInstBytesInt32(&ptr_plus_2, 2)) << 32) |
          ((int64_t) (NCInstBytesInt32(ptr, 4)));
    }
    case 8: {
      NCInstBytesPtr ptr_plus_4;
      NCInstBytesPtrInitInc(&ptr_plus_4, ptr, 4);
      return ((int64_t) (NCInstBytesInt32(&ptr_plus_4, 4)) << 32) |
          ((int64_t) (NCInstBytesInt32(ptr, 4)));
    }
    default:
      CHECK(0); /* Fail -- should not happen. */
      return -1;
  }
}

void NCInstBytesAdvance(NCInstBytesPtr* ptr, int n) {
  int index = ptr->pos + n;
  if (index < NCBUF_BYTES_LENGTH(ptr->bytes)) {
    ptr->pos = index;
  } else {
    ptr->bytes->memory->error_fn(NCInstBufferOverflow, ptr->bytes->memory);
  }
}

int NCInstBytesLength(const NCInstBytesPtr* ptr) {
  return (int) ptr->bytes->length - (int) ptr->pos;
}
