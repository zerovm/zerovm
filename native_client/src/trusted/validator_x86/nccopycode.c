/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * nccopycode.c
 * Copies two code streams in a thread-safe way
 *
 */

#include "native_client/src/include/portability.h"

#if NACL_WINDOWS == 1
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"
#include "native_client/src/trusted/validator/ncvalidate.h"

#if NACL_ARCH(NACL_TARGET_ARCH) == NACL_x86
# if NACL_TARGET_SUBARCH == 32
#  include "native_client/src/trusted/validator/x86/ncval_seg_sfi/ncdecode.h"
#  include "native_client/src/trusted/validator/x86/ncval_seg_sfi/ncvalidate.h"
# elif NACL_TARGET_SUBARCH == 64
#  include "native_client/src/trusted/validator/x86/decoder/nc_inst_iter.h"
#  include "native_client/src/trusted/validator/x86/decoder/nc_inst_state_internal.h"
#  include "native_client/src/trusted/validator/x86/nacl_cpuid.h"
#  include "native_client/src/trusted/validator/x86/ncval_reg_sfi/ncval_decode_tables.h"
#  include "native_client/src/trusted/validator/x86/nc_segment.h"
# else
#  error "Unknown Platform"
# endif
#else
# error "Unknown Platform"
#endif

/* x86 HALT opcode */
static const uint8_t kNaClFullStop = 0xf4;

/*
 * Max size of aligned writes we may issue to code without syncing.
 * 8 is a safe value according to:
 *   [1] Advance Micro Devices Inc. AMD64 Architecture Program-
 *   mers Manual Volume 1: Application Programming, 2009.
 *   [2] Intel Inc. Intel 64 and IA-32 Architectures Software Developers
 *   Manual Volume 3A: System Programming Guide, Part 1, 2010.
 *   [3] Vijay Sundaresan, Daryl Maier, Pramod Ramarao, and Mark
 *   Stoodley. Experiences with multi-threading and dynamic class
 *   loading in a java just-in-time compiler. Code Generation and
 *   Optimization, IEEE/ACM International Symposium on, 0:87–
 *   97, 2006.
 */
static const int kTrustAligned = 8;

/*
 * Max size of unaligned writes we may issue to code.
 * Empirically this may be larger, however no docs to support it.
 * 1 means disabled.
 */
static const int kTrustUnaligned = 1;

/*
 * Boundary no write may ever cross.
 * On AMD machines must be leq 8.  Intel machines leq cache line.
 */
static const int kInstructionFetchSize = 8;

/* defined in nccopycode_stores.S */
void _cdecl onestore_memmove4(uint8_t* dst, uint8_t* src);

/* defined in nccopycode_stores.S */
void _cdecl onestore_memmove8(uint8_t* dst, uint8_t* src);

static INLINE int IsAligned(uint8_t *dst, int size, int align) {
  uintptr_t startaligned = ((uintptr_t)dst)        & ~(align-1);
  uintptr_t stopaligned  = ((uintptr_t)dst+size-1) & ~(align-1);
  return startaligned == stopaligned;
}

/*
 * Test if it is safe to issue a unsynced change to dst/size using a
 * writesize write.  Outputs the offset to start the write at.
 * 1 if it is ok, 0 if it is not ok.
 */
static int IsTrustedWrite(uint8_t *dst,
                          int size,
                          int writesize,
                          intptr_t* offset) {
  if (size > writesize) {
    return 0;
  }
  if (!IsAligned(dst, size, kInstructionFetchSize)) {
    return 0;
  }
  if (writesize <= kTrustAligned && IsAligned(dst, size, writesize)) {
    /* aligned write is trusted */
    *offset = (intptr_t) dst & (writesize - 1);
    return 1;
  }
  if (writesize <= kTrustUnaligned) {
    /* unaligned write is trusted */
    *offset = 0;
    return 1;
  }
  return 0;
}

/* this is global to prevent a (very smart) compiler from optimizing it out */
void* g_squashybuffer = NULL;
char g_firstbyte = 0;

static Bool SerializeAllProcessors() {
  /*
   * We rely on the OS mprotect() call to issue interprocessor interrupts,
   * which will cause other processors to execute an IRET, which is
   * serializing.
   *
   * This code is based on two main considerations:
   * 1. Only switching the page from exec to non-exec state is guaranteed
   * to invalidate processors' instruction caches.
   * 2. It's bad to have a page that is both writeable and executable,
   * even if that happens not simultaneously.
   */

  int size = NACL_MAP_PAGESIZE;
  if (NULL == g_squashybuffer) {
    if ((0 != NaCl_page_alloc(&g_squashybuffer, size)) ||
        (0 != NaCl_mprotect(g_squashybuffer, size, PROT_READ|PROT_WRITE))) {
      NaClLog(0,
              ("SerializeAllProcessors: initial squashybuffer allocation"
               " failed\n"));
      return FALSE;
    }

    NaClFillMemoryRegionWithHalt(g_squashybuffer, size);
    g_firstbyte = *(char *) g_squashybuffer;
    NaClLog(0, "SerializeAllProcessors: g_firstbyte is %d\n", g_firstbyte);
  }

  if ((0 != NaCl_mprotect(g_squashybuffer, size, PROT_READ|PROT_EXEC))) {
    NaClLog(0,
            ("SerializeAllProcessors: interprocessor interrupt"
             " generation failed: could not reverse shield polarity (1)\n"));
    return FALSE;
  }
  /*
   * Make a read to ensure that the potential kernel laziness
   * would not affect this hack.
   */
  if (*(char *) g_squashybuffer != g_firstbyte) {
    NaClLog(0,
            ("SerializeAllProcessors: interprocessor interrupt"
             " generation failed: could not reverse shield polarity (2)\n"));
    NaClLog(0, "SerializeAllProcessors: g_firstbyte is %d\n", g_firstbyte);
    NaClLog(0, "SerializeAllProcessors: *g_squashybuffer is %d\n",
            *(char *) g_squashybuffer);
    return FALSE;
  }
  /*
   * We would like to set the protection to PROT_NONE, but on Windows
   * there's an ugly hack in NaCl_mprotect where PROT_NONE can result
   * in MEM_DECOMMIT, causing the contents of the page(s) to be lost!
   */
  if (0 != NaCl_mprotect(g_squashybuffer, size, PROT_READ)) {
    NaClLog(0,
            ("SerializeAllProcessors: interprocessor interrupt"
             " generation failed: could not reverse shield polarity (3)\n"));
    return FALSE;
  }
  return TRUE;
}

/*
 * Copy a single instruction, avoiding the possibility of other threads
 * executing a partially changed instruction.
 */
static Bool CopyInstructionInternal(uint8_t *dst,
                                    uint8_t *src,
                                    uint8_t sz) {
  intptr_t offset = 0;
  uint8_t *firstbyte_p = dst;

  while (sz > 0 && dst[0] == src[0]) {
    /* scroll to first changed byte */
    dst++, src++, sz--;
  }

  if (sz == 0) {
    /* instructions are identical, we are done */
    return TRUE;
  }

  while (sz > 0 && dst[sz-1] == src[sz-1]) {
    /* trim identical bytes at end */
    sz--;
  }

  if (sz == 1) {
    /* we assume a 1-byte change is atomic */
    *dst = *src;
  } else if (IsTrustedWrite(dst, sz, 4, &offset)) {
    uint8_t tmp[4];
    memcpy(tmp, dst-offset, sizeof tmp);
    memcpy(tmp+offset, src, sz);
    onestore_memmove4(dst-offset, tmp);
  } else if (IsTrustedWrite(dst, sz, 8, &offset)) {
    uint8_t tmp[8];
    memcpy(tmp, dst-offset, sizeof tmp);
    memcpy(tmp+offset, src, sz);
    onestore_memmove8(dst-offset, tmp);
  } else {
    /* the slow path, first flip first byte to halt*/
    uint8_t firstbyte = firstbyte_p[0];
    firstbyte_p[0] = kNaClFullStop;

    if (!SerializeAllProcessors()) return FALSE;

    /* copy the rest of instruction */
    if (dst == firstbyte_p) {
      /* but not the first byte! */
      firstbyte = *src;
      dst++, src++, sz--;
    }
    memcpy(dst, src, sz);

    if (!SerializeAllProcessors()) return FALSE;

    /* flip first byte back */
    firstbyte_p[0] = firstbyte;
  }
  return TRUE;
}

#if NACL_TARGET_SUBARCH == 32

/*
 * Copy a single instruction, avoiding the possibility of other threads
 * executing a partially changed instruction.
 */
static Bool CopyInstruction(NCDecoderStatePair* tthis,
                            NCDecoderInst *dinst_old,
                            NCDecoderInst *dinst_new) {
  NCRemainingMemory* mem_old = &dinst_old->dstate->memory;
  NCRemainingMemory* mem_new = &dinst_new->dstate->memory;

  return CopyInstructionInternal(mem_old->mpc,
                                 mem_new->mpc,
                                 mem_old->read_length);
}

int NCCopyCode(uint8_t *dst, uint8_t *src, NaClPcAddress vbase,
               size_t sz, int bundle_size) {
  NCDecoderState dst_dstate;
  NCDecoderInst  dst_inst;
  NCDecoderState src_dstate;
  NCDecoderInst  src_inst;
  NCDecoderStatePair pair;
  int result = 0;

  NCDecoderStateConstruct(&dst_dstate, dst, vbase, sz, &dst_inst, 1);
  NCDecoderStateConstruct(&src_dstate, src, vbase, sz, &src_inst, 1);
  NCDecoderStatePairConstruct(&pair, &dst_dstate, &src_dstate);
  pair.action_fn = CopyInstruction;
  if (NCDecoderStatePairDecode(&pair)) result = 1;
  NCDecoderStatePairDestruct(&pair);
  NCDecoderStateDestruct(&src_dstate);
  NCDecoderStateDestruct(&dst_dstate);

  return result;
}

NaClValidationStatus NACL_SUBARCH_NAME(ApplyValidatorCopy,
                                       NACL_TARGET_ARCH,
                                       NACL_TARGET_SUBARCH)
    (enum NaClSBKind sb_kind,
     uintptr_t guest_addr,
     uint8_t *data_old,
     uint8_t *data_new,
     size_t size,
     int bundle_size) {
  NaClValidationStatus status = NaClValidationFailedNotImplemented;
  assert(NACL_SB_DEFAULT == sb_kind);
  if (bundle_size == 16 || bundle_size == 32) {
    NaClCPUData cpu_data;
    NaClCPUDataGet(&cpu_data);
    if (!NaClArchSupported(&cpu_data)) {
      status = NaClValidationFailedCpuNotSupported;
    } else {
      status = ((0 == NCCopyCode(data_old, data_new, guest_addr,
                                size, bundle_size))
                ? NaClValidationFailed : NaClValidationSucceeded);
    }
  }
  return status;
}

#elif NACL_TARGET_SUBARCH == 64

int NaClCopyCodeIter(uint8_t *dst, uint8_t *src,
                     NaClPcAddress vbase, size_t size) {
  NaClSegment segment_old, segment_new;
  NaClInstIter *iter_old, *iter_new;
  NaClInstState *istate_old, *istate_new;
  int still_good = 1;

  NaClSegmentInitialize(dst, vbase, size, &segment_old);
  NaClSegmentInitialize(src, vbase, size, &segment_new);

  iter_old = NaClInstIterCreate(kNaClValDecoderTables, &segment_old);
  if (NULL == iter_old) return 0;
  iter_new = NaClInstIterCreate(kNaClValDecoderTables, &segment_new);
  if (NULL == iter_new) {
    NaClInstIterDestroy(iter_old);
    return 0;
  }
  while (1) {
    /* March over every instruction, which means NaCl pseudo-instructions are
     * treated as multiple instructions.  Checks in NaClValidateCodeReplacement
     * guarantee that only valid replacements will happen, and no pseudo-
     * instructions should be touched.
     */
    if (!(NaClInstIterHasNext(iter_old) && NaClInstIterHasNext(iter_new))) {
      if (NaClInstIterHasNext(iter_old) || NaClInstIterHasNext(iter_new)) {
        NaClLog(LOG_ERROR,
                "Segment replacement: copy failed: iterators "
                "length mismatch\n");
        still_good = 0;
      }
      break;
    }
    istate_old = NaClInstIterGetState(iter_old);
    istate_new = NaClInstIterGetState(iter_new);
    if (istate_old->bytes.length != istate_new->bytes.length ||
        iter_old->memory.read_length != iter_new->memory.read_length ||
        istate_new->vpc != istate_old->vpc) {
      /* Sanity check: this should never happen based on checks in
       * NaClValidateInstReplacement.
       */
      NaClLog(LOG_ERROR,
              "Segment replacement: copied instructions misaligned\n");
      still_good = 0;
      break;
    }
    /* Replacing all modified instructions at once could yield a speedup here
     * as every time we modify instructions we must serialize all processors
     * twice.  Re-evaluate if code modification performance is an issue.
     */
    if (!CopyInstructionInternal(iter_old->memory.mpc,
                                 iter_new->memory.mpc,
                                 iter_old->memory.read_length)) {
      NaClLog(LOG_ERROR,
              "Segment replacement: copy failed: unable to copy instruction\n");
      still_good = 0;
      break;
    }
    NaClInstIterAdvance(iter_old);
    NaClInstIterAdvance(iter_new);
  }

  NaClInstIterDestroy(iter_old);
  NaClInstIterDestroy(iter_new);
  return still_good;
}

NaClValidationStatus NACL_SUBARCH_NAME(ApplyValidatorCopy,
                                       NACL_TARGET_ARCH,
                                       NACL_TARGET_SUBARCH)
    (enum NaClSBKind sb_kind,
     uintptr_t guest_addr,
     uint8_t *data_old,
     uint8_t *data_new,
     size_t size,
     int bundle_size) {
  NaClValidationStatus status = NaClValidationFailedNotImplemented;
  assert(NACL_SB_DEFAULT == sb_kind);
  if (bundle_size == 16 || bundle_size == 32) {
    NaClCPUData cpu_data;
    NaClCPUDataGet(&cpu_data);
    if (!NaClArchSupported(&cpu_data)) {
      status = NaClValidationFailedCpuNotSupported;
    } else {
      status = ((0 == NaClCopyCodeIter(data_old, data_new, guest_addr, size))
                ? NaClValidationFailed : NaClValidationSucceeded);
    }
  }
  return status;
}

#endif
