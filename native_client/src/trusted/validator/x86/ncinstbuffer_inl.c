/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* ncinstbuffer-inl.h - Holds nline functions for commonly used (simple)
 * functions in ncinstbuffer.h. Used to speed up code. Inlineed routines
 * correspond to the following functions in ncinstbuffer.h, but with an
 * 'Inline' suffix:
 *
 *   NCRemainingMemoryAdvance
 *   NCRemainingMemoryReset
 *   NCRemainingMemoryLookahead
 *   NCRemainingMemoryRead
 *   NCInstBytesPeek
 *   NCInstByte
 *   NCInstBytesRead
 *   NCInstBytesReadBytes
 *   NCInstBytesReset
 *   NCInstBytesInit
 *   NCInstBytesByte
 *
 * See ncinstbuffer.h for comments on how to use the corresponding inline
 * functions.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCINSTBUFFER_INL_C__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCINSTBUFFER_INL_C__

#include "native_client/src/trusted/validator/x86/ncinstbuffer.h"

/* Constant NCBUF_CLEAR_CACHE controls the behaviour of the buffer containing
 * the sequence of parsed bytes. Turn it on (1) to fill unused bytes with the
 * constant zero, and to allow access to all bytes in the sequence of parsed
 * bytes. Turn it off (0) to force access to only include the actual parsed
 * bytes.
 *
 * Note: Ideally, we would like to turn this feature off. However, the current
 * instruction parser (in ncdecode.c) and corresponding printer (in
 * ncdis_util.c) are problematic. The parser allows a partial match of
 * an instruction, without verifying that ALL necessary bytes are there. The
 * corresponding printer, assumes that only complete matches (during parsing)
 * were performed. The result is that the code sometimes assumes that many
 * more bytes were parsed than were actually parsed.
 *
 * To quickly fix the code so that it doesn't do illegal memory accesses, but
 * has consistent behaviour, the flag is currently sets NCBUF_CLEAR_CACHE to 1.
 *
 * To debug this problem, set the flag NCBUF_CLEAR_CACHE to 0.
 *
 * TODO(karl) Fix the parser/printer so that NCBUF_CLEAR_CACHE can be set to 0.
 */
#define NCBUF_CLEAR_CACHE 1

/* Defines the number of bytes in the buffer. */
#if NCBUF_CLEAR_CACHE
#define NCBUF_BYTES_LENGTH(bytes) MAX_INST_LENGTH
#else
#define NCBUF_BYTES_LENGTH(bytes) (bytes)->length
#endif

/* The constant to return if memory overflow occurs. */
# define NC_MEMORY_OVERFLOW 0

/* Returns the next byte in memory, or 0x00 if there are no more
 * bytes in memory.
 */
static INLINE uint8_t NCRemainingMemoryPeekInline(NCRemainingMemory* memory) {
  return (memory->cur_pos >= memory->mlimit)
      ? NC_MEMORY_OVERFLOW : *(memory->cur_pos);
}

/* Starts a new instruction at the current position in the memory
 * segment.
 */
static INLINE void NCRemainingMemoryAdvanceInline(NCRemainingMemory* memory) {
  memory->mpc = memory->cur_pos;
  memory->read_length = 0;
  memory->overflow_count = 0;
}

/* Moves back to the beginning of the current instruction in
 * the memory segment.
 */
static INLINE void NCRemainingMemoryResetInline(NCRemainingMemory* memory) {
  memory->cur_pos = memory->mpc;
  memory->next_byte = NCRemainingMemoryPeekInline(memory);
  memory->read_length = 0;
  memory->overflow_count = 0;
}

/* Looks ahead N bytes into the memory, and returns the corresponding
 * byte, or 0x00 if at the end of memory. i is zero-based.
 */
static INLINE uint8_t NCRemainingMemoryLookaheadInline(
    NCRemainingMemory* memory, ssize_t n) {
  if ((memory->cur_pos + n) < memory->mlimit) {
    return memory->cur_pos[n];
  } else {
    return NC_MEMORY_OVERFLOW;
  }
}

/* Reads and returns the next byte in the memory segment. Returns 0x00 if at
 * the end of the memory segment.
 */
static INLINE uint8_t NCRemainingMemoryReadInline(NCRemainingMemory* memory) {
  uint8_t byte = memory->next_byte;
  if (memory->cur_pos == memory->mlimit) {
    /* If reached, next_byte already set to 0 by last read. */
    if (0 == memory->overflow_count) {
      memory->error_fn(NCRemainingMemoryOverflow, memory);
    }
    memory->overflow_count++;
  } else {
    memory->read_length++;
    memory->cur_pos++;
    memory->next_byte = NCRemainingMemoryPeekInline(memory);
  }
  return byte;
}

/* Peek ahead and return the nth (zero based) byte from the current position
 * in the sequence of bytes being parsed.
 */
static INLINE uint8_t NCInstBytesPeekInline(NCInstBytes* bytes, ssize_t n) {
  return NCRemainingMemoryLookaheadInline(bytes->memory, n);
}

/* Peek at the nth character in the sequence of bytes being parsed (independent
 * of the current position).
 */
static INLINE uint8_t NCInstByteInline(NCInstBytes* bytes, ssize_t n) {
  if (n < bytes->length)  {
    return bytes->byte[n];
  } else {
    return NCRemainingMemoryLookaheadInline(bytes->memory, n - bytes->length);
  }
}

/* Reads a byte from the memory segment and adds it to the instruction buffer.
 * Returns the read byte.
 * Note: Assumes that NCInstBytesInitMemory has already been called to associate
 * memory.
 */
static INLINE uint8_t NCInstBytesReadInline(NCInstBytes* bytes) {
  uint8_t byte = NCRemainingMemoryReadInline(bytes->memory);
  if (bytes->length < MAX_INST_LENGTH) {
    bytes->byte[bytes->length++] = byte;
  } else {
    bytes->memory->error_fn(NCInstBufferOverflow, bytes->memory);
  }
  return byte;
}

/* Reads n bytes from the memory segment and adds it to the instruction buffer.
 * Note: Assumes that NCInstBytesInitMemory has already been called to associate
 * memory.
 */
static INLINE void NCInstBytesReadBytesInline(ssize_t n, NCInstBytes* bytes) {
  ssize_t i;
  for (i = 0; i < n; ++i) {
    NCInstBytesReadInline(bytes);
  }
}

/* Resets bytes back to the beginning of the current instruction. */
static INLINE void NCInstBytesResetInline(NCInstBytes* buffer) {
#if NCBUF_CLEAR_CACHE
  int i;
  for (i = 0; i < MAX_INST_LENGTH; ++i) {
    buffer->byte[i] = 0;
  }
#endif
  NCRemainingMemoryResetInline(buffer->memory);
  buffer->length = 0;
}

/* Initializes the instruction buffer as the empty buffer, and
 * advances the memory segment so that one is beginning the
 * parsing of the current instruction at the current position
 * in the memory segment.
 * Note: Assumes that NCInstBytesInitMemory has already been called to associate
 * memory.
 */
static INLINE void NCInstBytesInitInline(NCInstBytes* buffer) {
#if NCBUF_CLEAR_CACHE
  int i;
  for (i = 0; i < MAX_INST_LENGTH; ++i) {
    buffer->byte[i] = 0;
  }
#endif
  NCRemainingMemoryAdvanceInline(buffer->memory);
  buffer->length = 0;
}

/* Returns the indexed byte pointed to by the instruction buffer pointer. */
static INLINE uint8_t NCInstBytesByteInline(const NCInstBytesPtr* ptr, int n) {
  int index = ptr->pos + n;
  if (index < NCBUF_BYTES_LENGTH(ptr->bytes)) {
    return ptr->bytes->byte[index];
  } else {
    ptr->bytes->memory->error_fn(NCInstBufferOverflow, ptr->bytes->memory);
    return 0;
  }
}

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCINSTBUFFER_INL_C__ */
