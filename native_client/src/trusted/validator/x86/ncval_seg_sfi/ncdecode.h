/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * ncdecode.h - table driven decoder for Native Client.
 *
 * This header file contains type declarations and constants
 * used by the decoder input table
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_SEG_SFI_NCDECODE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_SEG_SFI_NCDECODE_H_

#include "native_client/src/shared/utils/types.h"
#include "native_client/src/trusted/validator/x86/error_reporter.h"
#include "native_client/src/trusted/validator/x86/ncinstbuffer.h"
#include "native_client/src/trusted/validator/x86/x86_insts.h"

EXTERN_C_BEGIN

struct NCDecoderInst;
struct NCDecoderState;

/* Function type for a decoder action. Returns TRUE if action
 * was applied successfully.
 */
typedef Bool (*NCDecoderStateAction)(const struct NCDecoderInst* dinst);

/* Function type for other decoder state methods. */
typedef void (*NCDecoderStateMethod)(struct NCDecoderState* vstate);

typedef enum {
  NOGROUP = 0,
  GROUP1,
  GROUP2,
  GROUP3,
  GROUP4,
  /* these comments facilitate counting */
  GROUP5,
  GROUP6,
  GROUP7,
  GROUP8,
  GROUP9,
  /* these comments facilitate counting */
  GROUP10,
  GROUP11,
  GROUP12,
  GROUP13,
  GROUP14,
  /* these comments facilitate counting */
  GROUP15,
  GROUP16,
  GROUP17,
  GROUP1A,
  GROUPP
} NaClMRMGroups;
/* kModRMOpcodeGroups doesn't work as a const int since it is used */
/* as an array dimension */
#define kNaClMRMGroupsRange 20

/* Define the maximum value that can be encoded in the modrm mod field. */
#define kModRMOpcodeGroupSize 8

/* Define the maximum register value that can be encoded into the opcode
 * byte.
 */
#define kMaxRegisterIndexInOpcode 7

/* information derived from the opcode, wherever it happens to be */
typedef enum {
  IMM_UNKNOWN = 0,
  IMM_NONE = 1,
  IMM_FIXED1 = 2,
  IMM_FIXED2 = 3,
  IMM_FIXED3 = 4,
  IMM_FIXED4 = 5,
  IMM_DATAV = 6,
  IMM_ADDRV = 7,
  IMM_GROUP3_F6 = 8,
  IMM_GROUP3_F7 = 9,
  IMM_FARPTR = 10,
  IMM_MOV_DATAV,     /* Special case for 64-bits MOVs (b8 through bf). */
  /* Don't add to this enum without update kNCDecodeImmediateTypeRange */
  /* and updating the tables below which are sized using this constant */
} NCDecodeImmediateType;
#define kNCDecodeImmediateTypeRange 12

/* 255 will force an error */
static const uint8_t kImmTypeToSize66[kNCDecodeImmediateTypeRange] =
  { 0, 0, 1, 2, 3, 4, 2, (NACL_TARGET_SUBARCH == 64 ? 8 : 4), 0, 0, 6, 2};
static const uint8_t kImmTypeToSize67[kNCDecodeImmediateTypeRange] =
  { 0, 0, 1, 2, 3, 4, 4, 2, 0, 0, 4, 4};
static const uint8_t kImmTypeToSize[kNCDecodeImmediateTypeRange] =
  { 0, 0, 1, 2, 3, 4, 4, (NACL_TARGET_SUBARCH == 64 ? 8 : 4), 0, 0, 6, 4 };

/* Defines how to decode operands for byte codes. */
typedef enum {
  /* Assume the default size of the operands is 64-bits (if
   * not specified in prefix bits).
   */
  DECODE_OPS_DEFAULT_64,
  /* Assume the default size of the operands is 32-bits (if
   * not specified in prefix bits).
   */
  DECODE_OPS_DEFAULT_32,
  /* Force the size of the operands to 64 bits (prefix bits are
   * ignored).
   */
  DECODE_OPS_FORCE_64
} DecodeOpsKind;

/* Models information on an x86-32 bit instruction. */
struct OpInfo {
  NaClInstType insttype;
  uint8_t hasmrmbyte;   /* 1 if this inst has an mrm byte, else 0 */
  uint8_t immtype;      /* IMM_NONE, IMM_FIXED1, etc. */
  uint8_t opinmrm;      /* set to 1..8 if you must find opcode in MRM byte */
};

/* Models a node in a trie of NOP instructions. */
typedef struct NCNopTrieNode {
  /* The matching byte for the trie node. */
  uint8_t matching_byte;
  /* The matching modeled nop, if byte matched. */
  struct OpInfo *matching_opinfo;
  /* Node to match remaining bytes. */
  struct NCNopTrieNode* success;
  /* Node to match remaining bytes. */
  struct NCNopTrieNode* fail;
} NCNopTrieNode;

/* Models a parsed x86-32 bit instruction. */
struct InstInfo {
  /* The bytes used to parse the x86-32 instruction (may have added
   * zero filler if the instruction straddles the memory segment).
   */
  NCInstBytes bytes;
  /* The number of prefix bytes in the instruction. */
  uint8_t prefixbytes;  /* 0..4 */
  /* Number of opcode bytes in the instruction. */
  uint8_t num_opbytes;
  /* non-zero if the instruction contains an SIB byte. */
  uint8_t hassibbyte;
  /* The ModRm byte. */
  uint8_t mrm;
  /* A NCDecodeImmediateType describing the type of immediate value(s)
   * the instruction has.
   */
  uint8_t immtype;
  /* The number of bytes that define the immediate value(s). */
  uint8_t immbytes;
  /* The number of displacement bytes defined by the instruction. */
  uint8_t dispbytes;
  /* The set of prefix masks defined by the prefix bytes. */
  uint32_t prefixmask;
  /* The prefix form used to select multibyte instructions, or 0 if
   * not used. That is, if 66, f2, or f3 is used to select the instruction,
   * then this value is non-zero. For example SSE3 instructions.
   */
  uint32_t opcode_prefixmask;
  /* True if it has a rex prefix. */
  uint8_t rexprefix;
};

/* Models data collected about the parsed instruction. */
typedef struct NCDecoderInst {
  /* The virtual (pc) address of the instruction. */
  NaClPcAddress vpc;
  /* The instruction rule used to decode the instruction. */
  const struct OpInfo* opinfo;
  /* The low level details of the instructionm, extracted during parsing. */
  struct InstInfo inst;
  /* Pointer to bytes of the parsed instruction (int inst) for easier access. */
  const NCInstBytesPtr inst_bytes;
  /* The decoder state the instruction appears in. */
  struct NCDecoderState* dstate;
  /* Corresopnding index of this instruction wrt to inst_buffer in
   * in the corresponding decoder state NCDecoderState.
   */
  size_t inst_index;
  /* The number of instructions parsed so far (including this instrruction).
   * Used to detect when one tries to get a previous instruction that doesn't
   * exist.
   */
  size_t inst_count;
} NCDecoderInst;

/* Given a (decoded) instruction, return the instruction that appeared
 * n elements before it, or NULL if no such instruction exists.
 *
 * Parameters:
 *    dinst - The instruction to look up relative to.
 *    n - number of elements back to look.
 */
extern NCDecoderInst *PreviousInst(const NCDecoderInst* dinst, int n);

/* Models decoding instructions in a memory region.
 *
 * Note: This struct is modeling a notion of a (virtual) base class to parse
 * a window of k instructions. In this model, we consider NCDecoderState a
 * class that can be (singly) inherited by derived classes. This code
 * assumes that the "this" pointer can be cast to a derived class
 * using a C cast. This implies that derived classes should have the
 * field NCDecoderState as its first field.
 *
 * Typical use is:
 *
 *    NCDecoderState dstate;
 *    NCDecoder inst_buffer[BUF_SIZE]; // window of BUF_SIZE instructions.
 *    NCDecoderStateConstruct(&dstate, mbase, vbase, size,
 *                            inst_buffer, BUF_SIZE);
 *    NCDecoderStateDecode(&dstate);
 *    NCDecoderStateDestruct(&dstate);
 *
 * Note: The old API for this class is further down in this file,
 * and should be considered deprecated.
 */
typedef struct NCDecoderState {
  /* PROTECTED: */

  /* The instruction buffer is an array of instructions, used
   * by the decoder to define a window of decoded instructions.
   * This window automatically moves as instructions are decoded
   * so that one can always see the current decoded instruction,
   * and some (fixed) number of previously decoded instructions.
   */
  NCDecoderInst* inst_buffer;

  /* The number of elements in inst_buffer. Must be greater than zero. */
  size_t inst_buffer_size;

  /* Remaining memory to decode. It is allocated on
   * the stack to make it thread-local, and included here
   * so that all decoder states have access to it.
   */
  NCRemainingMemory memory;

  /* The begining of the memory segment to decode. */
  uint8_t* mbase;

  /* The (virtual) base address of the memory segment. */
  NaClPcAddress vbase;

  /* The number of bytes in the memory segment. */
  NaClMemorySize size;

  /* The index of the current instruction within inst_buffer. */
  size_t cur_inst_index;

  /* Holds the error reporting object to use. */
  NaClErrorReporter* error_reporter;

  /* Member function to apply actions to a decoded instruction. */
  NCDecoderStateAction action_fn;

  /* Member function to process new segment. */
  NCDecoderStateMethod new_segment_fn;

  /* Member function called to report an error with the validity of the
   * memory segment.
   */
  NCDecoderStateMethod segmentation_error_fn;

  /* Member function called to report other errors while processing the
   * memory segment.
   */
  NCDecoderStateMethod internal_error_fn;
} NCDecoderState;

/*
 * Construct a decoder state.
 *
 * Parameters are:
 *   this  - The instance to be constructed.
 *   mbase - The begining of the memory segment to decode.
 *   vbase - The (virtual) base address of the memory segment.
 *   sz - The number of bytes in the memory segment.
 *
 * Note: Constructors of subclasses of NCDecoderState should
 * call this constructor first, to initialize the decoder state.
 */
extern void NCDecoderStateConstruct(NCDecoderState* tthis,
                                    uint8_t* mbase, NaClPcAddress vbase,
                                    NaClMemorySize sz,
                                    NCDecoderInst* inst_buffer,
                                    size_t inst_buffer_size);

/* Define an error reporter to use to report error messages.
 * Note: By default, a decoder state uses the null error reporter,
 * which doesn't report error messages.
 *
 * WARNING: Be sure the error reporter is expecting a NCDecoderInst* for
 * the print_inst method.
 */
void NCDecoderStateSetErrorReporter(NCDecoderState* tthis,
                                    NaClErrorReporter* reporter);


/* A default, null error reporter for a NCDecoderInst* instruction. */
extern NaClErrorReporter kNCNullErrorReporter;

/*
 * Decodes the memory segment associated with the decoder state.
 * Returns TRUE if able to apply action to all decoded instructions.
 *
 * Parameters are:
 *   this  - The decoder state.
 */
extern Bool NCDecoderStateDecode(NCDecoderState* tthis);

/*
 * Destruct a decoder state.
 *
 * Parameters are:
 *   this  - The decoder state.
 *
 * Note: Destructors of subclasses of NCDecoderState should
 * call this destructor last, after the subinstance has been destructed.
 */
extern void NCDecoderStateDestruct(NCDecoderState* tthis);

struct NCDecoderStatePair;

/* Models a method that does a compare/update on a pair of instructions from
 * the pairwise instruction decoder. Returns true if the action succeeded.
 */
typedef Bool (*NCDecoderStatePairAction)(struct NCDecoderStatePair* tthis,
                                         struct NCDecoderInst* dinst_old,
                                         struct NCDecoderInst* dinst_new);

/* Models decoding a pair of instruction segments, compariing/updating
 * them as appropriate. Assumes that two instruction segments are the same,
 * except for some (constant-sized) changes. At the instruction level,
 * the instruction lengths are assumed to be the same. Typically, this is
 * because the one instruction segment was an updated version of a
 * previous instruction segment.
 *
 * Typical use is:
 *
 * NCDecoderState dstate_old;
 * NCDecoderState dstate_new;
 * NCDecoderStatePair dstate_pair;
 * ... Code that constructs dstate_old and dstate_new.
 * NCDecoderStatePair Construct(&dstate_pair, &dstate_old, &dstate_new);
 * NCDecoderStatePairDecode(&dstate_pair);
 * NCDecoderStatePairDestruct(&dstate_pair);
 */
typedef struct NCDecoderStatePair {
  /* PROTECTED: */

  /* The old decoder state. */
  NCDecoderState* old_dstate;

  /* The new decoder state. */
  NCDecoderState* new_dstate;

  /* The (virtual method) action to apply to each instruction. */
  NCDecoderStatePairAction action_fn;
} NCDecoderStatePair;

/*
 * Construct a decoder state pair.
 *
 * Parameters are:
 *    tthis - The decoder state pair to construct.
 *    old_dstate - A constructed old decoder state to use.
 *    new_dstate - A constructed new decoder state to use.
 *
 * Note: Constructors of subclasses of NCDecoderStatePair should
 * call this constructor first, to initialize the decoder pair fields.
 */
extern void NCDecoderStatePairConstruct(NCDecoderStatePair* tthis,
                                        NCDecoderState* old_dstate,
                                        NCDecoderState* new_dstate);

/*
 * Decode the memory segments in each instruction state, applying
 * the appropriate action on each instruction till either:
 * (1) The instruction lengths differ.
 * (2) The action returns false.
 * Returns true if no instruction lengths differ, and the action
 * returns true for all found instructions.
 */
extern Bool NCDecoderStatePairDecode(NCDecoderStatePair* tthis);

/*
 * Destruct a decoder state pair.
 *
 * Note: Destructors of subclasses of NCDecoderStatePair should
 * call this distructor last, after the subinstance has been destructed.
 */
extern void NCDecoderStatePairDestruct(NCDecoderStatePair* tthis);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NCVAL_SEG_SFI_NCDECODE_H_ */
