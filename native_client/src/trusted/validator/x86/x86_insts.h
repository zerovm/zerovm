/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * x86_insts.h - Holds common utilties for decoding x86 instructions.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_X86_INSTS_H__
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_X86_INSTS_H__

#include "native_client/src/include/portability.h"

/* Readability macros for bitset testing. */
#define NaClHasBit(set, bit) ((set) & (bit))
#define NaClExcludesBit(set, bit) (~(set) & bit)

/* Readability macros for changing bitsets. */
#define NaClAddBits(set, bits) (set |= (bits))
#define NaClRemoveBits(set, bits) (set &= ~(bits))

EXTERN_C_BEGIN

/* Defines the corresponding byte encodings for each of the prefixes. */
#define kValueSEGCS  0x2e
#define kValueSEGSS  0x36
#define kValueSEGFS  0x64
#define kValueSEGGS  0x65
#define kValueDATA16 0x66
#define kValueADDR16 0x67
#define kValueREPNE  0xf2
#define kValueREP    0xf3
#define kValueLOCK   0xf0
#define kValueSEGES  0x26
#define kValueSEGDS  0x3e

/* Using a bit mask here. Hopefully nobody will be offended.
 * Prefix usage: 0x2e and 0x3e are used as branch prediction hints
 *               0x64 and 0x65 needed for TLS
 *               0x26 and 0x36 shouldn't be needed
 * These are #defines, not const ints, because they are used
 * for array initialization
 */
#define kPrefixSEGCS  0x0001  /* 0x2e */
#define kPrefixSEGSS  0x0002  /* 0x36 */
#define kPrefixSEGFS  0x0004  /* 0x64 */
#define kPrefixSEGGS  0x0008  /* 0x65 */
#define kPrefixDATA16 0x0010  /* 0x66 - OKAY */
#define kPrefixADDR16 0x0020  /* 0x67 - disallowed */
#define kPrefixREPNE  0x0040  /* 0xf2 - OKAY */
#define kPrefixREP    0x0080  /* 0xf3 - OKAY */
#define kPrefixLOCK   0x0100  /* 0xf0 - OKAY */
#define kPrefixSEGES  0x0200  /* 0x26 - disallowed */
#define kPrefixSEGDS  0x0400  /* 0x3e - disallowed */
#define kPrefixREX    0x1000  /* 0x40 - 0x4f Rex prefix */

/* a new enumerated type for instructions.
 * Note: Each enumerate type is marked with one of the following symbols,
 * defining the validator it us used for:
 *    32 - The x86-32 validator.
 *    64 - The x86-64 validator.
 *    Both - Both the x86-32 and the x86-64 validators.
 * Note: The code for the x86-64 validator is being cleaned up, and there
 * are still uses of the "32" tag for x86 instructions.
 * TODO(karl) - Fix this comment when modeling for the x86-64 has been cleaned
 * up.
 */
typedef enum {
  NACLi_UNDEFINED = 0, /* uninitialized space; should never happen */ /* Both */
  NACLi_ILLEGAL,      /* not allowed in NaCl */                       /* Both */
  NACLi_INVALID,      /* not valid on any known x86 */                /* Both */
  NACLi_SYSTEM,       /* ring-0 instruction, not allowed in NaCl */   /* Both */
  NACLi_NOP,          /* Predefined nop instruction sequence. */      /* 32 */
  NACLi_UD2,          /* Predefined ud2 instruction sequence. */      /* 32 */
  NACLi_386,          /* an allowed instruction on all i386 implementations */
                                                                      /* Both */
                      /* subset of i386 that allows LOCK prefix. NOTE:
                       * This is only used for the 32 bit validator. The new
                       * 64 bit validator uses "InstFlag(OpcodeLockable)"
                       * to communicate this (which separates the CPU ID
                       * information from whether the instruction is lockable.
                       * Hopefully, in future releases, this enumerated type
                       * will be removed.
                       */
  NACLi_386L,                                                         /* 32 */
  NACLi_386R,         /* subset of i386 that allow REP prefix */      /* 32 */
  NACLi_386RE,        /* subset of i386 that allow REPE/REPZ prefixes */
                                                                      /* 32 */
  NACLi_JMP8,                                                         /* 32 */
  NACLi_JMPZ,                                                         /* 32 */
  NACLi_INDIRECT,                                                     /* 32 */
  NACLi_OPINMRM,                                                      /* 32 */
  NACLi_RETURN,                                                       /* 32 */
  NACLi_SFENCE_CLFLUSH,                                               /* Both */
  NACLi_CMPXCHG8B,                                                    /* Both */
  NACLi_CMPXCHG16B,   /* 64-bit mode only, illegal for NaCl */        /* Both */
  NACLi_CMOV,                                                         /* Both */
  NACLi_RDMSR,                                                        /* Both */
  NACLi_RDTSC,                                                        /* Both */
  NACLi_RDTSCP,  /* AMD only */                                       /* Both */
  NACLi_SYSCALL, /* AMD only; equivalent to SYSENTER */               /* Both */
  NACLi_SYSENTER,                                                     /* Both */
  NACLi_X87,                                                          /* Both */
  NACLi_MMX,                                                          /* Both */
  NACLi_MMXSSE2, /* MMX with no prefix, SSE2 with 0x66 prefix */      /* Both */
  NACLi_3DNOW,   /* AMD only */                                       /* Both */
  NACLi_EMMX,    /* Cyrix only; not supported yet */                  /* Both */
  NACLi_E3DNOW,  /* AMD only */                                       /* Both */
  NACLi_SSE,                                                          /* Both */
  NACLi_SSE2,    /* no prefix => MMX; prefix 66 => SSE; */            /* Both */
                 /* f2, f3 not allowed unless used for opcode selection */
  NACLi_SSE2x,   /* SSE2; prefix 66 required!!! */                    /* 32 */
  NACLi_SSE3,                                                         /* Both */
  NACLi_SSE4A,   /* AMD only */                                       /* Both */
  NACLi_SSE41,                                                        /* Both */
  NACLi_SSE42,                                                        /* Both */
  NACLi_MOVBE,                                                        /* Both */
  NACLi_POPCNT,                                                       /* Both */
  NACLi_LZCNT,                                                        /* Both */
  NACLi_LONGMODE,/* AMD only? */                                      /* 32 */
  NACLi_SVM,     /* AMD only */                                       /* Both */
  NACLi_SSSE3,                                                        /* Both */
  NACLi_3BYTE,                                                        /* 32 */
  NACLi_FCMOV,                                                        /* 32 */
  NACLi_VMX,                                                          /* 64 */
  NACLi_FXSAVE   /* SAVE/RESTORE xmm, mmx, and x87 state. */          /* 64 */
  /* NOTE: This enum must be kept consistent with kNaClInstTypeRange   */
  /* (defined below). */
  /* Note: If you change this enumerated type, be sure to update */
  /* kNaClInstTypeString in x86_insts.c. */
} NaClInstType;

#define kNaClInstTypeRange 47

/* Returns the print name for the enumerated type.*/
const char* NaClInstTypeString(NaClInstType typ);

/* Define the maximum register value that can be encoded into the opcode
 * byte.
 */
#define kMaxRegisterIndexInOpcode 7

extern const uint8_t kFirstX87Opcode;  /* 0xd8 */
extern const uint8_t kLastX87Opcode;   /* 0xdf */

/* Defines the opcode for the WAIT instruction.
 * Note: WAIT is an x87 instruction but not in the coproc opcode space.
 */
extern const uint8_t kWAITOp;  /* 0x9b */

#define NCDTABLESIZE 256

extern const uint8_t kTwoByteOpcodeByte1;  /* 0x0f */
extern const uint8_t k3DNowOpcodeByte2;    /* 0x0f */
extern const int kMaxPrefixBytes;      /* 4    */

/* Accessors for the ModRm byte. */
extern uint8_t modrm_mod(uint8_t modrm);
extern uint8_t modrm_rm(uint8_t modrm);
extern uint8_t modrm_reg(uint8_t modrm);
extern uint8_t modrm_opcode(uint8_t modrm);

/* Accessors for the Sib byte. */
extern uint8_t sib_ss(uint8_t sib);
extern uint8_t sib_index(uint8_t sib);
extern uint8_t sib_base(uint8_t sib);

/* Defines the range of rex prefix values. */
extern const uint8_t NaClRexMin;  /* 0x40 */
extern const uint8_t NaClRexMax;  /* 0x4F */

/* Accessors for each of the rex bits. */
uint8_t NaClRexW(uint8_t prefix);
uint8_t NaClRexR(uint8_t prefix);
uint8_t NaClRexX(uint8_t prefix);
uint8_t NaClRexB(uint8_t prefix);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_X86_INSTS_H__ */
