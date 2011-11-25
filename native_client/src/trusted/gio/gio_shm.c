/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include <errno.h>
#include <string.h>

#include "native_client/src/include/portability.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"

#include "native_client/src/trusted/gio/gio_shm.h"
#include "native_client/src/trusted/service_runtime/include/sys/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/sel_util.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_effector_trusted_mem.h"
#include "native_client/src/trusted/desc/nacl_desc_imc_shm.h"

/*
 * This code maps in GIO_SHM_WINDOWSIZE bytes at a time for doing
 * "I/O" from/to the shared memory object.  This value must be an
 * integer multiple of NACL_MAP_PAGESIZE.
 */
#define GIO_SHM_WINDOWSIZE  (16 * NACL_MAP_PAGESIZE)

/*
 * Release current window if it exists, then map in window at the
 * provided new_window_offset.  This is akin to filbuf.
 *
 * Preconditions: 0 == (new_win_offset & (NACL_MAP_PAGESIZE - 1))
 *                new_win_offset < self->shm_sz
 */
static int NaClGioShmSetWindow(struct NaClGioShm  *self,
                               size_t             new_win_offset) {
  int       rv;
  uintptr_t map_result;
  size_t    actual_len;

  NaClLog(4,
          "NaClGioShmSetWindow: new_win_offset 0x%"NACL_PRIxS"\n",
          new_win_offset);
  if (0 != (new_win_offset & (NACL_MAP_PAGESIZE - 1))) {
    NaClLog(LOG_FATAL,
            ("NaClGioShmSetWindow: internal error, requested"
             " new window offset 0x%"NACL_PRIxS" is not aligned.\n"),
            new_win_offset);
  }

  if (new_win_offset >= self->shm_sz) {
    NaClLog(LOG_FATAL,
            ("NaClGioShmSetWindow: setting window beyond end of shm object"
             " offset 0x%"NACL_PRIxS", size 0x%"NACL_PRIxS"\n"),
            new_win_offset, self->shm_sz);
  }

  if (NULL != self->cur_window) {
    rv = (*((struct NaClDescVtbl const *) self->shmp->base.vtbl)->
          UnmapUnsafe)(self->shmp,
                       (struct NaClDescEffector *) &self->eff,
                       (void *) self->cur_window,
                       self->window_size);
    if (0 != rv) {
      NaClLog(LOG_FATAL,
              "NaClGioShmSetWindow: UnmapUnsafe returned %d\n",
              rv);
    }
  }
  self->cur_window = NULL;
  self->window_size = 0;

  /*
   * The Map virtual function will NOT pad space beyond the end of the
   * memory mapping object with zero-filled pages.  This is done for
   * user code in nacl_syscall_common.c(NaClCommonSysMmap), and the
   * Map virtual function exposes the behavioral inconsistencies wrt
   * allowing but ignoring mapping an offset beyond the end of file
   * (linux) versus disallowing the mapping (MapViewOfFileEx).
   *
   * Here, we know the actual size of the shm object, and can deal
   * with it.
   */
  actual_len = GIO_SHM_WINDOWSIZE;
  if (actual_len > self->shm_sz - new_win_offset) {
    actual_len = self->shm_sz - new_win_offset;
  }
  map_result =
      (*((struct NaClDescVtbl const *) self->shmp->base.vtbl)->
       Map)(self->shmp,
            (struct NaClDescEffector *) &self->eff,
            (void *) NULL,
            actual_len,
            NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
            NACL_ABI_MAP_SHARED,
            (nacl_off64_t) new_win_offset);
  NaClLog(4,
          "NaClGioShmSetWindow: Map returned 0x%"NACL_PRIxPTR"\n",
          map_result);
  if (NACL_ABI_MAP_FAILED == (void *) map_result) {
    return 0;
  }

  self->cur_window = (char *) map_result;
  self->window_size = actual_len;
  self->window_offset = new_win_offset;

  return 1;
}

static ssize_t NaClGioShmReadOrWrite(struct Gio *vself,
                                     void       *buf,
                                     size_t     count,
                                     int        is_write) {
  struct NaClGioShm *self = (struct NaClGioShm *) vself;
  size_t            new_window_offset;
  size_t            transfer;
  size_t            window_end;
  size_t            window_remain;
  size_t            sofar;

  NaClLog(4,
          ("NaClGioShmReadOrWrite: 0x%"NACL_PRIxPTR","
           " 0x%"NACL_PRIxPTR", 0x%"NACL_PRIxS", %d\n"),
          (uintptr_t) vself,
          (uintptr_t) buf,
          count,
          is_write);
  sofar = 0;
  while (count > 0) {
    NaClLog(4, "NaClGioShmReadOrWrite: count 0x%"NACL_PRIxS"\n", count);
    if (self->io_offset >= self->shm_sz) {
      break;
    }
    NaClLog(4, "   cur_window 0x%"NACL_PRIxPTR"\n",
            (uintptr_t) self->cur_window);
    NaClLog(4, "    io_offset 0x%"NACL_PRIxS"\n", self->io_offset);
    NaClLog(4, "window_offset 0x%"NACL_PRIxS"\n", self->window_offset);
    if (NULL == self->cur_window
        || self->io_offset < self->window_offset
        || self->window_offset + self->window_size <= self->io_offset) {
      /*
       * io_offset is outside the window.  move the window so that
       * it's within.
       */
      NaClLog(4, "Seek required\n");

      new_window_offset = (self->io_offset
                           & (~(((size_t) NACL_MAP_PAGESIZE) - 1)));
      NaClLog(4, "new_window_offset 0x%"NACL_PRIxS"\n", new_window_offset);
      CHECK(0 == (new_window_offset &
                  (((size_t) NACL_MAP_PAGESIZE)-1)));
      if (!NaClGioShmSetWindow(self, new_window_offset)) {
        if (0 == sofar) {
          errno = EIO;
          sofar = -1;
        }
        return sofar;
      }
    } else {
      NaClLog(4, "no seek required\n");
    }
    NaClLog(4, "   cur_window 0x%"NACL_PRIxPTR"\n",
            (uintptr_t) self->cur_window);
    NaClLog(4, "    io_offset 0x%"NACL_PRIxS"\n", self->io_offset);
    NaClLog(4, "window_offset 0x%"NACL_PRIxS"\n", self->window_offset);

    CHECK(self->window_offset <= self->io_offset);
    CHECK(self->io_offset < self->window_offset + self->window_size);

    transfer = count;
    window_end = self->window_offset + self->window_size;
    if (window_end > self->shm_sz) {
      window_end = self->shm_sz;
    }
    window_remain = window_end - self->io_offset;

    NaClLog(4, "remaining in window 0x%"NACL_PRIxS"\n", window_remain);

    CHECK(window_remain <= GIO_SHM_WINDOWSIZE);

    if (transfer > window_remain) {
      transfer = window_remain;
    }

    NaClLog(4, "transfer 0x%"NACL_PRIxS"\n", transfer);

    if (is_write) {
      NaClLog(4,
              ("about to \"write\" memcpy(0x%"NACL_PRIxPTR", "
               " 0x%"NACL_PRIxPTR", 0x%"NACL_PRIxS" bytes)\n"),
              (uintptr_t) (self->cur_window
                           + (self->io_offset - self->window_offset)),
              (uintptr_t) buf,
              transfer);

      memcpy(self->cur_window + (self->io_offset - self->window_offset),
             buf,
             transfer);
    } else {
      NaClLog(4,
              ("about to \"read\" memcpy(0x%"NACL_PRIxPTR", "
               " 0x%"NACL_PRIxPTR", 0x%"NACL_PRIxS" bytes)\n"),
              (uintptr_t) buf,
              (uintptr_t) (self->cur_window
                           + (self->io_offset - self->window_offset)),
              transfer);

      memcpy(buf,
             self->cur_window + (self->io_offset - self->window_offset),
             transfer);
    }
    self->io_offset += transfer;
    sofar += transfer;

    buf = (void *)((uintptr_t) buf + transfer);
    count -= transfer;
  }

  return sofar;
}

static ssize_t NaClGioShmRead(struct Gio *vself,
                              void       *buf,
                              size_t     count) {
  return NaClGioShmReadOrWrite(vself, buf, count, 0);
}

static ssize_t NaClGioShmWrite(struct Gio *vself,
                               const void *buf,
                               size_t     count) {
  return NaClGioShmReadOrWrite(vself, (void *) buf, count, 1);
}

static off_t NaClGioShmSeek(struct Gio  *vself,
                            off_t       offset,
                            int         whence) {
  struct NaClGioShm *self = (struct NaClGioShm *) vself;
  size_t new_pos = (size_t) -1;

  NaClLog(4, "NaClGioShmSeek(0x%"NACL_PRIxPTR", %ld (0x%lx), %d)\n",
          (uintptr_t) vself, (long) offset, (long) offset, whence);
  /*
   * Note that if sizeof(new_pos) < sizeof(offset), we are dropping
   * high-order bits and we do not detect this.  However, the check
   * after the switch keeps the values somewhat sane: we will never
   * set the I/O offset to be outside the range [0, self->shm_sz].
   */
  switch (whence) {
    case SEEK_SET:
      new_pos = (size_t) offset;
      break;
    case SEEK_CUR:
      new_pos = self->io_offset + offset;
      break;
    case SEEK_END:
      new_pos = self->shm_sz + offset;
      break;
  }
  /* allow equality, so setting to the end of file is okay */
  if (self->shm_sz < new_pos) {
    NaClLog(4, " invalid offset\n");
    errno = EINVAL;
    return -1;
  }
  NaClLog(4, " setting to %ld (0x%lx)\n", (long) new_pos, (long) new_pos);
  /* sizeof(off_t) >= sizeof(size_t) */
  self->io_offset = new_pos;
  return (off_t) self->io_offset;
}

static int NaClGioShmFlush(struct Gio *vself) {
  UNREFERENCED_PARAMETER(vself);
  return 0;
}

static int NaClGioShmClose(struct Gio *vself) {
  struct NaClGioShm *self = (struct NaClGioShm *) vself;
  int               ret;

  if (NULL != self->cur_window) {
    ret = (*((struct NaClDescVtbl const *) self->shmp->base.vtbl)->
           UnmapUnsafe)(self->shmp,
                        (struct NaClDescEffector *) &self->eff,
                        (void *) self->cur_window,
                        NACL_MAP_PAGESIZE);
    if (ret < 0) {
      errno = EIO;
      return -1;
    }
  }
  self->cur_window = NULL;

  if (NULL == self->shmp) {
    NaClLog(LOG_ERROR, "NaClGioShmClose: double close detected\n");
    errno = EIO;
    return -1;
  }

  NaClDescUnref(self->shmp);
  self->shmp = NULL;  /* double close will fault */
  return 0;
}

static void NaClGioShmDtor(struct Gio *vself) {
  struct NaClGioShm *self = (struct NaClGioShm *) vself;

  /*
   * Users of Gio objects are expected to Close then Dtor, but Dtor
   * should cleanup regardless.
   */
  if (NULL != self->shmp) {
    if (-1 == (*vself->vtbl->Close)(vself)) {
      NaClLog(LOG_ERROR, "NaClGioShmDtor: auto Close failed!\n");
    }
  }

  (*self->eff.base.vtbl->Dtor)(&self->eff.base);

  self->shmp = NULL;
  self->base.vtbl = NULL;
}

const struct GioVtbl kNaClGioShmVtbl = {
  NaClGioShmRead,
  NaClGioShmWrite,
  NaClGioShmSeek,
  NaClGioShmFlush,
  NaClGioShmClose,
  NaClGioShmDtor,
};


static int NaClGioShmCtorIntern(struct NaClGioShm  *self,
                                struct NaClDesc    *shmp,
                                size_t             shm_size) {
  struct nacl_abi_stat  stbuf;
  int                   vfret;
  int                   rval = 0;

  self->base.vtbl = NULL;

  self->shmp = NULL;
  self->cur_window = NULL;

  if (0 != (vfret = (*((struct NaClDescVtbl const *) shmp->base.vtbl)->
                     Fstat)(shmp, &stbuf))) {
    NaClLog(1, "NaClGioShmCtorIntern: Fstat virtual function returned %d\n",
            vfret);
    goto cleanup;
  }
  /*
   * nacl_abi_off_t is signed 32-bit quantity, but we don't want to
   * hardwire in that knowledge here.
   *
   * size_t is unsigned, and may be 32-bits or 64-bits, depending on
   * the underlying host OS.
   *
   * we want to ensure that the shm's size, as reported by the desc
   * abstraction and thus is in nacl_abi_off_t, is at least that
   * claimed by the ctor argument.  so, if (as Integers)
   *
   *  stbuf.nacl_abi_st_size < shm_size
   *
   * holds, this is an error.  however, the value-preserving cast rule
   * makes this harder.
   *
   * Note that for signed sizes (ssize_t), the kernel ABI generally
   * only reserve -1 for error, and asking for an I/O operation via a
   * size_t that would succeed but yield a ssize_t return value that
   * is negative is okay, since -1 is never valid as an I/O size on a
   * von Neuman machine (except for a writev where the iov entries
   * overlap): there just isn't that much data to read/write, when the
   * instructions also take up space in the process address space.
   * Whether requiring the programmer to detect this corner case is
   * advisable is a different argument -- similar to negative ssize_t
   * sizes, the syscall can just succeed with a partial transfer to
   * avoid returning -1 on a success, just as we could avoid returning
   * negative values; in practice, we do the latter, since we often
   * see code written that tests for syscall error by comparing the
   * return value to see if it is less than zero, rather than if it is
   * equal to -1.
   */
  if (stbuf.nacl_abi_st_size < 0) {
    NaClLog(LOG_ERROR,
            ("NaClGioShmCtorIntern: actual shm size negative"
             " %"NACL_PRIdNACL_OFF"\n"),
            stbuf.nacl_abi_st_size);
    goto cleanup;
  }
  if (stbuf.nacl_abi_st_size <= (nacl_abi_off_t) SIZE_T_MAX
      && (size_t) stbuf.nacl_abi_st_size < shm_size) {
    NaClLog(LOG_ERROR,
            ("NaClGioShmCtorIntern: claimed shm file size greater than"
             " actual shm segment size, %"NACL_PRIuS" vs"
             " %"NACL_PRIuNACL_OFF"\n"),
            shm_size,
            stbuf.nacl_abi_st_size);
    goto cleanup;
  }
  if (OFF_T_MAX < SIZE_T_MAX && (size_t) OFF_T_MAX < shm_size) {
    NaClLog(LOG_ERROR,
            ("NaClGioShmCtorIntern: claimed shm file size greater than"
             " off_t max value, %"NACL_PRId64"\n"),
            (int64_t) OFF_T_MAX);
    goto cleanup;
  }

  self->shmp = NaClDescRef(shmp);

  self->io_offset = 0;
  self->shm_sz = shm_size;
  self->window_offset = 0;

  self->base.vtbl = &kNaClGioShmVtbl;

  if (!NaClGioShmSetWindow(self, 0)) {
    NaClLog(LOG_ERROR,
            ("NaClGioShmCtorIntern: initial seek to beginning failed\n"));
    self->base.vtbl = NULL;
    goto cleanup;
  }

  rval = 1;
 cleanup:
  if (!rval) {
    (*self->eff.base.vtbl->Dtor)(&self->eff.base);
  }
  return rval;
}

int NaClGioShmCtor(struct NaClGioShm  *self,
                   struct NaClDesc    *shmp,
                   size_t             shm_size) {

  int rv;

  CHECK(shm_size == NaClRoundAllocPage(shm_size));

  if (!NaClDescEffectorTrustedMemCtor(&self->eff)) {
    return 0;
  }

  rv = NaClGioShmCtorIntern(self, shmp, shm_size);

  if (!rv) {
    (*self->eff.base.vtbl->Dtor)(&self->eff.base);
  }
  return rv;
}

int NaClGioShmAllocCtor(struct NaClGioShm *self,
                        size_t            shm_size) {
  struct NaClDescImcShm *shmp;
  int                   rv;

  CHECK(shm_size == NaClRoundAllocPage(shm_size));

  if (!NaClDescEffectorTrustedMemCtor(&self->eff)) {
    return 0;
  }

  shmp = malloc(sizeof *shmp);
  if (NULL == shmp) {
    (*self->eff.base.vtbl->Dtor)(&self->eff.base);
    return 0;
  }
  if (!NaClDescImcShmAllocCtor(shmp, shm_size, /* executable= */ 0)) {
    (*self->eff.base.vtbl->Dtor)(&self->eff.base);
    free(shmp);
    return 0;
  }

  rv = NaClGioShmCtorIntern(self, (struct NaClDesc *) shmp, shm_size);

  if (!rv) {
    NaClDescUnref((struct NaClDesc *) shmp);
    free(shmp);
    (*self->eff.base.vtbl->Dtor)(&self->eff.base);
  }
  return rv;
}
