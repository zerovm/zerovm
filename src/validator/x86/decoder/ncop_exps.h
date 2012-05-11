/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * ncop-exps.h - Models x86 instructions using a vector containing
 * operand trees flattened using a pre-order walk.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NCOP_EXPS_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NCOP_EXPS_H_

#include "src/validator/x86/decoder/gen/ncop_expr_node_flag.h"
#include "src/validator/x86/decoder/gen/ncop_expr_node_kind.h"
#include "src/validator/x86/decoder/ncopcode_desc.h"

EXTERN_C_BEGIN

/* Defines the state used to match an instruction, while walking
 * instructions using the NaClInstIter.
 */
struct NaClInstState;

/* Returns the number of kids an ExprNodeKind has. */
int NaClExpKindRank(NaClExpKind kind);

/* Defines a bit set of ExprNodeFlags. */
typedef uint32_t NaClExpFlags;

/* Converts an NaClExpFlag enum to the corresponding bit in a NaClExpFlags
 * bit set.
 */
#define NACL_EFLAG(x) (((NaClExpFlags) 1) << (x))

/* Models the empty set of NaClExpFlags. */
#define NACL_EMPTY_EFLAGS ((NaClExpFlags) 0)

/* Print out the set of defined expr flags. */
void NaClPrintExpFlags(struct Gio* file, NaClExpFlags flags);

/* Defines a node in the vector of expressions, corresponding to the flattened
 * (preorder) tree.
 */
typedef struct NaClExp {
  /* A value associated with the kind. */
  uint64_t value;
  /* The type of node. */
  NaClExpKind kind;
  /* The set of flags associated with the node. */
  NaClExpFlags flags;
} NaClExp;

/* Maximum number of nodes allowed in the flattened (preorder) tree. */
#define NACL_MAX_EXPS 30

/* Defines the vector of expression nodes, corresponding to the flattened
 * (preorder) tree that defines the instruction expression.
 */
typedef struct NaClExpVector {
  Bool is_defined;  /* TRUE iff vector has been built. */
  uint32_t number_expr_nodes;
  NaClExp node[NACL_MAX_EXPS];
} NaClExpVector;

/* Returns the number of elements in the given vector, that the subtree
 * rooted at the given node covers.
 */
int NaClExpWidth(NaClExpVector* vector, int node);

/* Given the given index of the node in the vector, return the index of the
 * given (zero based) kid of the node.
 */
int NaClGetExpKidIndex(NaClExpVector* vector, int node, int kid);

/* Given an index in the vector, return the index to its parent.
 * Note: index must be > 0. Returns -1 if no parent defined.
 */
int NaClGetExpParentIndex(NaClExpVector* vector, int node);

/* Returns true if the index points to a constant that is negative. */
Bool NaClIsExpNegativeConstant(NaClExpVector* vector, int index);

/* Returns the index of the i-th occurrence of the given kind of node,
 * or -1 if no such node exists.
 */
int NaClGetNthExpKind(NaClExpVector* vector, NaClExpKind kind, int n);

/* Returns the register in the given node. */
NaClOpKind NaClGetExpRegister(NaClExp* node);

/* Returns the register in the given indexed node. */
NaClOpKind NaClGetExpVectorRegister(NaClExpVector* vector, int node);

/* Print out the contents of the given vector of nodes to the given file. */
void NaClExpVectorPrint(struct Gio* file, struct NaClInstState* state);

/* Print out the disassembled instruction in the given instruction state. */
void NaClInstStateInstPrint(struct Gio* file, struct NaClInstState* state);

/* Same functionality as NaClInstStateInstPrint(), but puts the
 * result in a string. This function is to be used for comparing output on
 * an instruction-at-a-time granularity. Do not use this fct except
 * for testing purposes.
 */
char* NaClInstStateInstructionToString(struct NaClInstState* state);

/* Generates the lower case name of the corresponding register into the
 * given character buffer. Returns the number of characters added to the buffer.
 * Returns zero if there is no corresponding (valid) register name for the reg
 * argument.
 */
size_t NaClOpRegName(NaClOpKind reg, char* buffer, size_t buffer_size);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_DECODER_NCOP_EXPS_H_ */
