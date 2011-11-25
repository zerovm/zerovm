/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Defines an instruction (decoder) iterator that processes code segments.
 *
 * The typical use is of the form:
 *
 *    NaClSegment segment;
 *    NaClInstIter* iter;
 *    NaClDecodeTables* tables;
 *    ...
 *    for (iter = NaClInstIterCreate(tables, &segment);
 *         NaClInstIterHasNext(iter);
 *         NaClInstIterAdvance(iter)) {
 *      NaClInstState* state = NaClInstIterGetState(iter);
 *      ...
 *    }
 *    NaClInstIterDestroy(iter);
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_ITER_h_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_ITER_h_

#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/validator/x86/decoder/ncopcode_desc.h"

EXTERN_C_BEGIN

/* Defines a code segment in the elf file. */
struct NaClSegment;

/* Defines the internal state associated with a parsed instruction.*/
struct NaClInstState;

struct NaClDecodeTables;

/* Defines the structure of an instruction iterator, which walks
 * the code segment, one instruction at a time.
 */
typedef struct NaClInstIter NaClInstIter;

/* Allocate and initialize an instruction iterator for the given code segment.
 */
NaClInstIter* NaClInstIterCreate(const struct NaClDecodeTables* decoder_tables,
                                 struct NaClSegment* segment);

/* Allocate and initialize an instruction iterator for the given code segment.
 * Add internal buffering that will allow one to look back the given number
 * of instructions from the current point of the iterator.
 */
NaClInstIter* NaClInstIterCreateWithLookback(
    const struct NaClDecodeTables* decoder_tables,
    struct NaClSegment* segment,
    size_t lookback_size);

/* Delete the instruction iterator created by either
 * NaClInstIterCreate or NaClInstIterCreateWithLookback.
 */
void NaClInstIterDestroy(NaClInstIter* iter);

/* Return true if there are more instructions in the code segment
 * to iterate over.
 */
Bool NaClInstIterHasNext(NaClInstIter* iter);

/* Return a state containing the currently matched instruction defined
 * by the given instruction iterator. Note: This instruction is only
 * valid until the next call to NaClInstIteratorAdvance.
 */
struct NaClInstState* NaClInstIterGetState(NaClInstIter* iter);

/* Returns true iff it is valid to look back the given distance. */
Bool NaClInstIterHasLookbackState(NaClInstIter* iter, size_t distance);

/* Return a state containing the instruction matched distance elements ago
 * from the currently matched instruction (zero corresponds to the currently
 * matched instruction). Note: This instruction is only valid until the next
 * call to NaClInstIteratorAdvance.
 */
struct NaClInstState* NaClInstIterGetLookbackState(
    NaClInstIter* iter, size_t distance);

/* Advance the iterator past the current instruction. */
void NaClInstIterAdvance(NaClInstIter* iter);

/* Returns the memory address of the beginning of the currently
 * matched instruction.
 */
uint8_t* NaClInstIterGetInstMemory(NaClInstIter* iter);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NC_INST_ITER_h_ */
