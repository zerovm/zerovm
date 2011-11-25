/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Transferrable shared memory objects.
 */

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_platform.h"

#include <stdlib.h>
#include <string.h>

#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_effector.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/desc/nacl_desc_imc_shm.h"

#include "native_client/src/shared/platform/nacl_find_addrsp.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"
#include "native_client/src/trusted/service_runtime/internal_errno.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

#ifndef SIZE_T_MAX
# define SIZE_T_MAX   (~(size_t) 0)
#endif

/*
 * This file contains the implementation of the NaClDescImcShm
 * subclass of NaClDesc.
 *
 * NaClDescImcShm is the subclass that wraps IMC shm descriptors.
 */

static struct NaClDescVtbl const kNaClDescImcShmVtbl;  /* fwd */

int NaClDescImcShmCtor(struct NaClDescImcShm  *self,
                       NaClHandle             h,
                       nacl_off64_t           size) {
  struct NaClDesc *basep = (struct NaClDesc *) self;

  /*
   * off_t is signed, but size_t are not; historically size_t is for
   * sizeof and similar, and off_t is also used for stat structure
   * st_size member.  This runtime test detects large object sizes
   * that are silently converted to negative values.
   */
  basep->base.vtbl = (struct NaClRefCountVtbl const *) NULL;
  if (size < 0 || SIZE_T_MAX < (uint64_t) size) {
    return 0;
  }

  if (!NaClDescCtor(basep)) {
    return 0;
  }
  self->h = h;
  self->size = size;
  basep->base.vtbl = (struct NaClRefCountVtbl const *) &kNaClDescImcShmVtbl;
  return 1;
}

int NaClDescImcShmAllocCtor(struct NaClDescImcShm  *self,
                            nacl_off64_t           size,
                            int                    executable) {
  NaClHandle h;
  int        rv;

  if (size < 0 || SIZE_T_MAX < (uint64_t) size) {
    NaClLog(4,
            "NaClDescImcShmAllocCtor: requested size 0x%08"NACL_PRIx64
            " (0x%08"NACL_PRId64") too large\n",
            size, size);
    return 0;
  }
  h = NaClCreateMemoryObject((size_t) size, executable);
  if (NACL_INVALID_HANDLE == h) {
    return 0;
  }
  if (0 == (rv = NaClDescImcShmCtor(self, h, size))) {
    (void) NaClClose(h);
  }
  return rv;
}

static void NaClDescImcShmDtor(struct NaClRefCount *vself) {
  struct NaClDescImcShm  *self = (struct NaClDescImcShm *) vself;

  (void) NaClClose(self->h);
  self->h = NACL_INVALID_HANDLE;
  vself->vtbl = (struct NaClRefCountVtbl const *) &kNaClDescVtbl;
  (*vself->vtbl->Dtor)(vself);
}

static uintptr_t NaClDescImcShmMap(struct NaClDesc         *vself,
                                   struct NaClDescEffector *effp,
                                   void                    *start_addr,
                                   size_t                  len,
                                   int                     prot,
                                   int                     flags,
                                   nacl_off64_t            offset) {
  struct NaClDescImcShm  *self = (struct NaClDescImcShm *) vself;

  int           rv;
  int           nacl_imc_prot;
  int           nacl_imc_flags;
  uintptr_t     addr;
  uintptr_t     end_addr;
  void          *result;
  nacl_off64_t  tmp_off64;
  off_t         tmp_off;

  NaClLog(4,
          "NaClDescImcShmMmap(,,0x%08"NACL_PRIxPTR",0x%"NACL_PRIxS","
          "0x%x,0x%x,0x%08"NACL_PRIxNACL_OFF64")\n",
          (uintptr_t) start_addr, len, prot, flags, offset);
  /*
   * shm must have NACL_ABI_MAP_SHARED in flags, and all calls through
   * this API must supply a start_addr, so NACL_ABI_MAP_FIXED is
   * assumed.
   */
  if (NACL_ABI_MAP_SHARED != (flags & NACL_ABI_MAP_SHARING_MASK)) {
    NaClLog(LOG_INFO,
            ("NaClDescImcShmMap: Mapping not NACL_ABI_MAP_SHARED,"
             " flags 0x%x\n"),
            flags);
    return -NACL_ABI_EINVAL;
  }
  if (0 != (NACL_ABI_MAP_FIXED & flags) && NULL == start_addr) {
    NaClLog(LOG_INFO,
            ("NaClDescImcShmMap: Mapping NACL_ABI_MAP_FIXED"
             " but start_addr is NULL\n"));
  }
  /* post-condition: if NULL == start_addr, then NACL_ABI_MAP_FIXED not set */

  /*
   * prot must not contain bits other than PROT_{READ|WRITE|EXEC}.
   */
  if (0 != (~(NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE | NACL_ABI_PROT_EXEC)
            & prot)) {
    NaClLog(LOG_INFO,
            "NaClDescImcShmMap: prot has other bits than"
            " PROT_{READ|WRITE|EXEC}\n");
    return -NACL_ABI_EINVAL;
  }
  /*
   * Map from NACL_ABI_ prot and flags bits to IMC library flags,
   * which will later map back into posix-style prot/flags on *x
   * boxen, and to MapViewOfFileEx arguments on Windows.
   */
  nacl_imc_prot = 0;
  if (NACL_ABI_PROT_READ & prot) {
    nacl_imc_prot |= NACL_PROT_READ;
  }
  if (NACL_ABI_PROT_WRITE & prot) {
    nacl_imc_prot |= NACL_PROT_WRITE;
  }
  if (NACL_ABI_PROT_EXEC & prot) {
    nacl_imc_prot |= NACL_PROT_EXEC;
  }
  nacl_imc_flags = NACL_MAP_SHARED;
  if (0 == (NACL_ABI_MAP_FIXED & flags)) {
    /* start_addr is a hint, and we just ignore the hint... */
    if (!NaClFindAddressSpace(&addr, len)) {
      NaClLog(1, "NaClDescImcShmMap: no address space?!?\n");
      return -NACL_ABI_ENOMEM;
    }
    start_addr = (void *) addr;
  }
  nacl_imc_flags |= NACL_MAP_FIXED;

  tmp_off64 = offset + len;
  /* just NaClRoundAllocPage, but in 64 bits */
  tmp_off64 = ((tmp_off64 + NACL_MAP_PAGESIZE - 1)
             & ~(uint64_t) (NACL_MAP_PAGESIZE - 1));
  if (tmp_off64 > INT32_MAX) {
    NaClLog(LOG_INFO,
            "NaClDescImcShmMap: total offset exceeds 32-bits\n");
    return -NACL_ABI_EOVERFLOW;
  }

  /*
   * For *x, we just map with MAP_FIXED and the kernel takes care of
   * atomically unmapping any existing memory.  For Windows, we must
   * unmap existing memory first, which creates a race condition,
   * where some other innocent thread puts some other memory into the
   * hole, and that memory becomes vulnerable to attack by the
   * untrusted NaCl application.
   *
   * For now, abort the process.  We will need to figure out how to
   * re-architect this code to do the address space move, since it is
   * deep surgery and we'll need to ensure that all threads have
   * stopped and any addresses derived from the old address space
   * would not be on any thread's call stack, i.e., stop the thread in
   * user space or before entering real service runtime code.  This
   * means that no application thread may be indefinitely blocked
   * performing a service call in the service runtime, since otherwise
   * there is no way for us to stop all threads.
   *
   * TODO(bsy): We will probably return an internal error code
   * -NACL_ABI_E_MOVE_ADDRESS_SPACE to ask the caller to do the address space
   * dance.
   */
  for (addr = (uintptr_t) start_addr,
           end_addr = addr + len,
           tmp_off = (off_t) offset;
       addr < end_addr;
       addr += NACL_MAP_PAGESIZE, tmp_off += NACL_MAP_PAGESIZE) {

    /*
     * Minimize the time between the unmap and the map for the same
     * page: we interleave the unmap and map for the pages, rather
     * than do all the unmap first and then do all of the map
     * operations.
     */
    if (0 !=
        (rv = (*effp->vtbl->UnmapMemory)(effp,
                                         addr,
                                         NACL_MAP_PAGESIZE))) {
      NaClLog(LOG_FATAL,
              ("NaClDescImcShmMap: error %d --"
               " could not unmap 0x%08"NACL_PRIxPTR", length 0x%x\n"),
              rv,
              addr,
              NACL_MAP_PAGESIZE);
    }

    result = NaClMap((void *) addr,
                     NACL_MAP_PAGESIZE,
                     nacl_imc_prot,
                     nacl_imc_flags,
                     self->h,
                     tmp_off);
    if (NACL_MAP_FAILED == result) {
      return -NACL_ABI_E_MOVE_ADDRESS_SPACE;
    }
    if (0 != (NACL_ABI_MAP_FIXED & flags) && result != (void *) addr) {
      NaClLog(LOG_FATAL,
              ("NaClDescImcShmMap: NACL_MAP_FIXED but"
               " got 0x%08"NACL_PRIxPTR" instead of 0x%08"NACL_PRIxPTR"\n"),
              (uintptr_t) result, addr);
    }
  }
  return (uintptr_t) start_addr;
}

static int NaClDescImcShmUnmapCommon(struct NaClDesc         *vself,
                                     struct NaClDescEffector *effp,
                                     void                    *start_addr,
                                     size_t                  len,
                                     int                     safe_mode) {
  int       retval;
  uintptr_t addr;
  uintptr_t end_addr;

  UNREFERENCED_PARAMETER(vself);

  retval = -NACL_ABI_EINVAL;

  for (addr = (uintptr_t) start_addr, end_addr = addr + len;
       addr < end_addr;
       addr += NACL_MAP_PAGESIZE) {
    int       status;

#if NACL_WINDOWS
    /*
     * On windows, we must unmap "properly", since overmapping will
     * not tear down existing page mappings.
     */
#elif NACL_LINUX || NACL_OSX
    if (!safe_mode) {
      /*
       * unsafe unmap always unmaps, w/o overmapping with anonymous
       * memory.  this is not necessary (nor desired) in safe_mode,
       * since overmapping with anonymous memory will atomically tear
       * down the mappings for these pages without leaving a timing
       * window open where the untrusted address space has unoccupied
       * page table entries.
       */
#else
# error "what platform?"
#endif
      /*
       * Do the unmap "properly" through NaClUnmap.
       */
      status = NaClUnmap((void *) addr, NACL_MAP_PAGESIZE);
      if (0 != status) {
        NaClLog(LOG_FATAL, "NaClDescImcShmUnmapCommon: NaClUnmap failed\n");
        goto done;
      }
#if NACL_LINUX || NACL_OSX
    }
#endif
    /* there's still a race condition */
    if (safe_mode) {
      uintptr_t result = (*effp->vtbl->MapAnonymousMemory)(effp,
                                                           addr,
                                                           NACL_MAP_PAGESIZE,
                                                           PROT_NONE);
      if (NaClPtrIsNegErrno(&result)) {
        NaClLog(LOG_ERROR, "NaClDescImcShmUnmapCommon: could not fill hole\n");
        retval = -NACL_ABI_E_MOVE_ADDRESS_SPACE;
        goto done;
      }
    }
  }
  retval = 0;
done:
  return retval;
}

static int NaClDescImcShmUnmapUnsafe(struct NaClDesc         *vself,
                                     struct NaClDescEffector *effp,
                                     void                    *start_addr,
                                     size_t                  len) {
  return NaClDescImcShmUnmapCommon(vself, effp, start_addr, len, 0);
}

static int NaClDescImcShmUnmap(struct NaClDesc         *vself,
                               struct NaClDescEffector *effp,
                               void                    *start_addr,
                               size_t                  len) {
  return NaClDescImcShmUnmapCommon(vself, effp, start_addr, len, 1);
}

static int NaClDescImcShmFstat(struct NaClDesc         *vself,
                               struct nacl_abi_stat    *stbp) {
  struct NaClDescImcShm  *self = (struct NaClDescImcShm *) vself;

  if (self->size > INT32_MAX) {
    return -NACL_ABI_EOVERFLOW;
  }

  stbp->nacl_abi_st_dev = 0;
  stbp->nacl_abi_st_ino = 0x6c43614e;
  stbp->nacl_abi_st_mode = (NACL_ABI_S_IFSHM |
                            NACL_ABI_S_IRUSR |
                            NACL_ABI_S_IWUSR);
  stbp->nacl_abi_st_nlink = 1;
  stbp->nacl_abi_st_uid = -1;
  stbp->nacl_abi_st_gid = -1;
  stbp->nacl_abi_st_rdev = 0;
  stbp->nacl_abi_st_size = (nacl_abi_off_t) self->size;
  stbp->nacl_abi_st_blksize = 0;
  stbp->nacl_abi_st_blocks = 0;
  stbp->nacl_abi_st_atime = 0;
  stbp->nacl_abi_st_mtime = 0;
  stbp->nacl_abi_st_ctime = 0;

  return 0;
}

static int NaClDescImcShmExternalizeSize(struct NaClDesc *vself,
                                         size_t          *nbytes,
                                         size_t          *nhandles) {
  struct NaClDescImcShm  *self = (struct NaClDescImcShm *) vself;

  *nbytes = sizeof self->size;
  *nhandles = 1;

  return 0;
}

static int NaClDescImcShmExternalize(struct NaClDesc           *vself,
                                     struct NaClDescXferState  *xfer) {
  struct NaClDescImcShm  *self = (struct NaClDescImcShm *) vself;

  *xfer->next_handle++ = self->h;
  memcpy(xfer->next_byte, &self->size, sizeof self->size);
  xfer->next_byte += sizeof self->size;
  return 0;
}

static struct NaClDescVtbl const kNaClDescImcShmVtbl = {
  {
    NaClDescImcShmDtor,
  },
  NaClDescImcShmMap,
  NaClDescImcShmUnmapUnsafe,
  NaClDescImcShmUnmap,
  NaClDescReadNotImplemented,
  NaClDescWriteNotImplemented,
  NaClDescSeekNotImplemented,
  NaClDescIoctlNotImplemented,
  NaClDescImcShmFstat,
  NaClDescGetdentsNotImplemented,
  NACL_DESC_SHM,
  NaClDescImcShmExternalizeSize,
  NaClDescImcShmExternalize,
  NaClDescLockNotImplemented,
  NaClDescTryLockNotImplemented,
  NaClDescUnlockNotImplemented,
  NaClDescWaitNotImplemented,
  NaClDescTimedWaitAbsNotImplemented,
  NaClDescSignalNotImplemented,
  NaClDescBroadcastNotImplemented,
  NaClDescSendMsgNotImplemented,
  NaClDescRecvMsgNotImplemented,
  NaClDescConnectAddrNotImplemented,
  NaClDescAcceptConnNotImplemented,
  NaClDescPostNotImplemented,
  NaClDescSemWaitNotImplemented,
  NaClDescGetValueNotImplemented,
};

int NaClDescImcShmInternalize(struct NaClDesc           **out_desc,
                              struct NaClDescXferState  *xfer) {
  int                   rv;
  struct NaClDescImcShm *ndisp;
  NaClHandle            h;
  nacl_off64_t          hsize;

  rv = -NACL_ABI_EIO;
  ndisp = NULL;

  if (xfer->next_handle == xfer->handle_buffer_end) {
    rv = -NACL_ABI_EIO;
    goto cleanup;
  }
  if (xfer->next_byte + sizeof ndisp->size > xfer->byte_buffer_end) {
    rv = -NACL_ABI_EIO;
    goto cleanup;
  }

  ndisp = malloc(sizeof *ndisp);
  if (NULL == ndisp) {
    rv = -NACL_ABI_ENOMEM;
    goto cleanup;
  }

  h = *xfer->next_handle;
  *xfer->next_handle++ = NACL_INVALID_HANDLE;
  memcpy(&hsize, xfer->next_byte, sizeof hsize);
  xfer->next_byte += sizeof hsize;

  if (0 == NaClDescImcShmCtor(ndisp, h, hsize)) {
    rv = -NACL_ABI_EIO;
    goto cleanup;
  }

  *out_desc = (struct NaClDesc *) ndisp;
  rv = 0;

cleanup:
  if (rv < 0) {
    free(ndisp);
  }
  return rv;
}
