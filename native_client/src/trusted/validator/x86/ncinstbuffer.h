/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * ncinstbuffer.h - Models bytes of matched instruction.
 *
 * Separates memory into two parts. The first is a block
 * of memory corresponding to the code segment, that needs
 * to be parsed to find instructions. The second is a copy
 * of the bytes read that corresponds to the current parsed
 * instruction.
 *
 * Note: We intentionally create a copy of the instruction bytes,
 * since the parsed instruction may be longer than the actual
 * code segment. In such cases, the memory segment reader automatically
 * fills in any extra reads with a zero byte value.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCINSTBUFFER_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCINSTBUFFER_H_

#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/validator/types_memory_model.h"

EXTERN_C_BEGIN

struct NCValidatorState;

/* Types of errors that can occur while parsing the memory segment. */
typedef enum {
  /* Read past the end of the memory segment. Note: this problem will also
   * be noted by byte overflow_count > 0 in the NCRemainingMemory structure.
   */
  NCRemainingMemoryOverflow,
  /* Too many bytes in an instruction, and can't fit in instruction buffer. */
  NCInstBufferOverflow,
  /* An unknown error occurred while parsing the memory segment. */
  NCUnknownMemoryError
} NCRemainingMemoryError;

struct NCRemainingMemory;

/* Type of routine to call if an error occurs parsing the memory segment
 * into an instruction buffer. Note: memory handling code will apply
 * recovery rules if the corresponding error function returns.
 */
typedef void (*NCRemainingMemoryErrorFn)(NCRemainingMemoryError error,
                                         struct NCRemainingMemory* memory);

/* Structure holding the memory segment to be parsed.
 * Note: In general, unless doing a post mortem on what happened,
 * you should not access fields. Rather use the API functions below.
 */
typedef struct NCRemainingMemory {
  /* The next byte in memory, or 0x00 if the memory segment has been
   * completely parsed.
   */
  uint8_t next_byte;
  /* Pointer to the next character to read from the memory segment. */
  uint8_t* cur_pos;
  /* Pointer to the byte after the end of the memory segment. */
  uint8_t* mlimit;
  /* Pointer to the beginning of the current instruction in memory
   * segment being parsed.
   */
  uint8_t* mpc;
  /* The actual number of bytes read from the memory segment for the
   * current instruction being parsed.
   */
  uint8_t read_length;
  /* The number of additional (0x00) bytes added, so that we could parse
   * the instruction.
   */
  uint8_t overflow_count;
  /* The error reporting function to use. Defaults to
   * NCRemainingMemoryReportError. Caller should set this immediately
   * after calling NCRemainingMemoryInit, if a different error reporting
   * function should be used.
   */
  NCRemainingMemoryErrorFn error_fn;
  /* Additional state associated with the memory segment.
   * Caller should set this immdiately after calling NCRemainingMemoryInit.
   * Used to communicate additional state to the validator error function.
   * Initialized to NULL in NCRemainingMemoryInit.
   * Note: The type is not specified since the notion of state is
   * decoder/validator specific, and corresponds to additional state
   * that may be needed by field error_fn. This field, by default is set
   * to NULL. When setting field error_fn, one can also set this field to
   * a corresponding structure that holds additional information for reporting
   * errors.
   */
  void* error_fn_state;
} NCRemainingMemory;

/* Initialize the (remaining) memory to the memory segment beginning
 * at memory_base, and containing size bytes.
 */
void NCRemainingMemoryInit(uint8_t* memory_base, NaClMemorySize size,
                           NCRemainingMemory* memory);

/* Returns the next byte in the memory segment, or 0x00 if at the
 * end of the memory segment.
 * Note: Same as NCRemainingMemoryLookahead(m, 0). Written as macro
 * for performance (using a cached field next_byte).
 */
#define NCRemainingMemoryGetNext(m) ((m)->next_byte)

/* Looks ahead N bytes into the memory, and returns the corresponding
 * byte, or 0x00 if at the end of memory. i is zero-based.
 */
uint8_t NCRemainingMemoryLookahead(NCRemainingMemory* memory, ssize_t n);

/* Reads and returns the next byte in the memory segment. Returns 0x00 if at
 * the end of the memory segment.
 */
uint8_t NCRemainingMemoryRead(NCRemainingMemory* memory);

/* Moves back to the beginning of the current instruction in
 * the memory segment.
 */
void NCRemainingMemoryReset(NCRemainingMemory* memory);

/* Starts a new instruction at the current position in the memory
 * segment.
 */
void NCRemainingMemoryAdvance(NCRemainingMemory* memory);

/* Default error message for type of error. */
const char* NCRemainingMemoryErrorMessage(NCRemainingMemoryError error);

/* Default memory segment reader error function. Prints out
 * string defined by NCRemainingMemoryErrorMessage to stdout.
 */
void NCRemainingMemoryReportError(NCRemainingMemoryError error,
                                  NCRemainingMemory* memory);

/* The maximum number of bytes to be recognized as an instruction. */
#define MAX_INST_LENGTH 15

/* Structure holding buffered bytes of a nacl instruction. This
 * structure is filled as the memory segment is parsed to recognize
 * the current instruction.
 */
typedef struct NCInstBytes {
  /* The sequence of bytes defining the instruction. */
  uint8_t byte[MAX_INST_LENGTH];
  /* The number of bytes (in buffer) defining the instruction. */
  uint8_t length;
  /* The memory associated with the bytes. Used to report
   * buffer overflows through the corresponding error function.
   */
  NCRemainingMemory* memory;
} NCInstBytes;

/* Associate memory with the corresponding instruction bytes. */
void NCInstBytesInitMemory(NCInstBytes* bytes, NCRemainingMemory* memory);

/* Resets bytes back to the beginning of the current instruction. */
void NCInstBytesReset(NCInstBytes* bytes);

/* Initializes the instruction buffer as the empty buffer, and
 * advances the memory segment so that one is beginning the
 * parsing of the current instruction at the current position
 * in the memory segment.
 * Note: Assumes that NCInstBytesInitMemory has already been called to associate
 * memory.
 */
void NCInstBytesInit(NCInstBytes* bytes);

/* Peek ahead and return the nth (zero based) byte from the current position
 * in the sequence of bytes being parsed.
 */
uint8_t NCInstBytesPeek(NCInstBytes* bytes, ssize_t n);

/* Peek at the nth character in the sequence of bytes being parsed (independent
 * of the current position).
 */
uint8_t NCInstByte(NCInstBytes* bytes, ssize_t n);

/* Reads a byte from the memory segment and adds it to the instruction buffer.
 * Returns the read byte.
 * Note: Assumes that NCInstBytesInitMemory has already been called to associate
 * memory.
 */
uint8_t NCInstBytesRead(NCInstBytes* bytes);

/* Reads n bytes from the memory segment and adds it to the instruction buffer.
 * Note: Assumes that NCInstBytesInitMemory has already been called to associate
 * memory.
 */
void NCInstBytesReadBytes(ssize_t n, NCInstBytes* bytes);

/* Structure holding pointer into bufferred bytes of a parsed instruction. */
typedef struct NCInstBytesPtr {
  /* The bytes the pointer is in. */
  const NCInstBytes* bytes;
  /* The index into bytes associated with the pointer. */
  uint8_t pos;
} NCInstBytesPtr;

/* Initializes the instruction bytes pointer to the given index in
 * the buffered bytes of a nacl instruction.
 */
void NCInstBytesPtrInit(NCInstBytesPtr* ptr, const NCInstBytes* bytes);

/* Initializes the instruction bytes pointer to the position of base, advanced
 * the given number of positions.
 */
void NCInstBytesPtrInitInc(NCInstBytesPtr* ptr, const NCInstBytesPtr* base,
                           int pos);

/* Returns the index position of the given bytes ptr in the instruction
 * bytes.
 */
uint8_t NCInstBytesPos(const NCInstBytesPtr* ptr);

/* Returns the 32-bit value pointed to by the ptr. */
int32_t NCInstBytesInt32(const NCInstBytesPtr* ptr);

/* Returns the indexed byte pointed to by the instruction buffer pointer. */
uint8_t NCInstBytesByte(const NCInstBytesPtr* ptr, int n);

/* Advances the instruction buffer pointer by the number of bytes. */
void NCInstBytesAdvance(NCInstBytesPtr* ptr, int n);

/* Returns the number of bytes left in the instruction buffer, based
 * on the pointer postiion.
 */
int NCInstBytesLength(const NCInstBytesPtr* ptr);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCINSTBUFFER_H_ */
