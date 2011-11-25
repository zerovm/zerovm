/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * Not Prometheus, just shared memory buffers.  We do the following:
 * we allocate and write into a NaClGioShm object until its allocation
 * size limit is reached.  Then, we double its size.  This means we
 * avoid quadratic copying.
 *
 * Also, we maintain the following invariant: all I/O operations are
 * done once, rather than split.  So if a write would grow a shm
 * object, we grow before doing the write.  This leads to more
 * copying, but makes the I/O operations simpler.
 */

#include <errno.h>

#include "native_client/src/trusted/gio/gio_shm_unbounded.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"


#if !defined(SIZE_T_MAX)
# define SIZE_T_MAX (~(size_t) 0)
#endif

static ssize_t NaClGioShmUnboundedRead(struct Gio  *vself,
                                       void        *buf,
                                       size_t      count) {
  struct NaClGioShmUnbounded *self = (struct NaClGioShmUnbounded *) vself;
  ssize_t                     got;
  size_t bytes_avail;

  NaClLog(4,
          ("NaClGioShmUnboundedRead(0x%"NACL_PRIxPTR","
           " 0x%"NACL_PRIxPTR", 0x%"NACL_PRIxS")\n"),
          (uintptr_t) vself, (uintptr_t) buf, count);
  /* are we at the end, or did we seek pass the end? */
  if (self->io_offset >= self->shm_written) {
    NaClLog(4, "io_offset 0x%"NACL_PRIxS", shm_written 0x%"NACL_PRIxS"\n",
            self->io_offset, self->shm_written);
    NaClLog(4, "I/O pointer outside of valid region, returning EOF\n");
    return 0;  /* EOF */
  }
  bytes_avail = self->shm_written - self->io_offset;
  if (count > bytes_avail) {
    count = bytes_avail;
  }
  NaClLog(4,
          ("NaClGioShmUnboundedRead: reading from underlying"
           " NaClGioShm 0x%"NACL_PRIxPTR"\n"),
          (uintptr_t) self->ngsp);
  got = (*self->ngsp->base.vtbl->Read)(&self->ngsp->base, buf, count);
  NaClLog(4,
          ("NaClGioShmUnboundedRead: got 0x%"NACL_PRIxS" bytes\n"),
          got);
  if (-1 != got) {
    self->io_offset += (size_t) got;
  }
  return got;
}

static void GioCopy(struct Gio  *dst,
                    struct Gio  *src,
                    size_t      nbytes) {
  uint8_t buf[4096];
  uint8_t *bufp;
  ssize_t got;
  ssize_t this_copy;
  size_t  ask;
  off_t   cur_offset;

  NaClLog(3,
          ("GioCopy: dst 0x%"NACL_PRIxPTR
           ", src 0x%"NACL_PRIxPTR", nbytes 0x%"NACL_PRIxS"\n"),
          (uintptr_t) dst,
          (uintptr_t) src,
          nbytes);
  cur_offset = (*src->vtbl->Seek)(src, 0, SEEK_CUR);
  if (-1 == cur_offset) {
    NaClLog(LOG_FATAL,
            "NaClGioShmUnbounded::GioCopy: could not find source ptr\n");
  }
  if (-1 == (*src->vtbl->Seek)(src, 0, SEEK_SET)) {
    NaClLog(LOG_FATAL,
            "NaClGioShmUnbounded::GioCopy: could not rewind source\n");
  }
  if (-1 == (*dst->vtbl->Seek)(dst, 0, SEEK_SET)) {
    NaClLog(LOG_FATAL,
            "NaClGioShmUnbounded::GioCopy: could not rewind destination\n");
  }
  /*
   * This copy process will dirty every page.  An optimization for
   * sparse data would check the result of a Read for all-zeros and
   * Seek the corresponding number of bytes forward.  This is,
   * however, unlikely to be a common enough case in our projected use
   * cases.
   */
  while (nbytes > 0) {
    ask = sizeof buf;
    if (ask > nbytes) {
      ask = nbytes;
    }
    NaClLog(5,
            "GioCopy: copying 0x%"NACL_PRIxS" bytes, 0x%"NACL_PRIxS" remains\n",
            ask,
            nbytes);
    got = (*src->vtbl->Read)(src, buf, ask);
    if (got <= 0 || (size_t) got > ask) {
      NaClLog(LOG_FATAL,
              "NaClGioShmUnbounded::GioCopy: read failed, %"NACL_PRIdS"\n",
              got);
    }
    this_copy = got;

    for (ask = (size_t) got, bufp = buf;
         ask > 0;
         bufp += got, ask -= got) {
      got = (*dst->vtbl->Write)(dst, buf, ask);
      if (got <= 0 || (size_t) got > ask) {
        NaClLog(LOG_FATAL,
                "NaClGioShmUnbounded::GioCopy: write failed, %"NACL_PRIdS"\n",
                got);
      }
    }
    nbytes -= this_copy;
  }
  if (-1 == (*dst->vtbl->Seek)(dst, cur_offset, SEEK_SET)) {
    NaClLog(LOG_FATAL,
            "NaClGioShmUnbounded::GioCopy: could not seek dst ptr\n");
  }
}

static ssize_t NaClGioShmUnboundedWrite(struct Gio  *vself,
                                        void const   *buf,
                                        size_t       count) {
  struct NaClGioShmUnbounded  *self = (struct NaClGioShmUnbounded *) vself;
  size_t                      io_offset;
  ssize_t                     retval;
  size_t                      new_avail_sz;
  size_t                      new_size;
  struct NaClGioShm           *ngsp;

  NaClLog(4,
          ("NaClGioShmUnboundedWrite(0x%"NACL_PRIxPTR","
           " 0x%"NACL_PRIxPTR", 0x%"NACL_PRIxS")\n"),
          (uintptr_t) vself, (uintptr_t) buf, count);
  if (SIZE_T_MAX - self->io_offset < count) {
    errno = EINVAL;
    return -1;
  }

  /*
   * where we'll end up when the I/O is done
   */
  io_offset = self->io_offset + count;

  /*
   * For sequential I/O, an "if" suffices.  For writes that occur
   * after a seek, however, we may need to double more than once.
   */
  for (new_avail_sz = self->shm_avail_sz;
       new_avail_sz < io_offset;
       new_avail_sz = new_size) {
    if (SIZE_T_MAX / 2 >= new_avail_sz) {
      new_size = 2 * new_avail_sz;
    } else {
      new_size = SIZE_T_MAX - NACL_MAP_PAGESIZE;
      ++new_size;
      /*
       * We could return -1 w/ ENOMEM here as well, but let's peg the
       * max size.
       */
      if (new_size <= new_avail_sz) {
        /*
         * We get equality if we try to expand again.
         */
        errno = ENOMEM;
        return -1;
      }
    }
  }
  if (new_avail_sz != self->shm_avail_sz) {
    /*
     * Replace the ngsp with one that is the new size.  This means
     * that there is a temporary 3x VM hit in the worst case.  This
     * should be primarily paging space, since I/O between the
     * NaClGioShm object should use relatively little RAM.  It will
     * trash the cache, however.  Hopefully the shm object is in the
     * buffer cache, and we're just mapping in chunks of it into our
     * address space.  This is a bit more explicit than mmapping both
     * source and destinaton objects completely and using madvise with
     * MADV_SEQUENTIAL -- and likely less efficient -- but avoids
     * OS-specific calls.
     */

    ngsp = malloc(sizeof *ngsp);

    if (NULL == ngsp) {
      errno = ENOMEM;
      return -1;
    }
    if (!NaClGioShmAllocCtor(ngsp, new_avail_sz)) {
      free(ngsp);
      errno = ENOMEM;
      return -1;
    }
    GioCopy((struct Gio *) ngsp, (struct Gio *) self->ngsp, self->shm_avail_sz);
    self->shm_avail_sz = new_avail_sz;

    if (-1 == (*self->ngsp->base.vtbl->Close)(&self->ngsp->base)) {
      NaClLog(LOG_ERROR,
              "NaClGioShmUnboundedWrite: close of src temporary failed\n");
    }
    (*self->ngsp->base.vtbl->Dtor)(&self->ngsp->base);
    free(self->ngsp);
    self->ngsp = ngsp;
    ngsp = NULL;
  }

  retval = (*self->ngsp->base.vtbl->Write)(&self->ngsp->base,
                                           buf, count);
  if (-1 != retval) {
    if ((size_t) retval > count) {
      errno = EIO;  /* internal error */
      return -1;
    }
    io_offset = self->io_offset + retval;

    if (io_offset > self->shm_written) {
      self->shm_written = io_offset;
      NaClLog(4,
              ("UPDATE: io_offset 0x%"NACL_PRIxS
               ", shm_written 0x%"NACL_PRIxS"\n"),
              self->io_offset, self->shm_written);
    }
    self->io_offset = io_offset;
  }

  NaClLog(4, "io_offset 0x%"NACL_PRIxS", shm_written 0x%"NACL_PRIxS"\n",
          self->io_offset, self->shm_written);

  return retval;
}

static off_t NaClGioShmUnboundedSeek(struct Gio *vself,
                                     off_t      offset,
                                     int        whence) {
  struct NaClGioShmUnbounded  *self = (struct NaClGioShmUnbounded *) vself;
  off_t                       new_pos;

  NaClLog(4, "NaClGioShmUnboundedSeek(0x%"NACL_PRIxPTR", %ld, %d)\n",
          (uintptr_t) vself, (long) offset, whence);
  NaClLog(4, "io_offset 0x%"NACL_PRIxS", shm_written 0x%"NACL_PRIxS"\n",
          self->io_offset, self->shm_written);
  new_pos = (*self->ngsp->base.vtbl->Seek)(&self->ngsp->base, offset, whence);
  if (-1 != new_pos) {
    NaClLog(4, " setting io_offset to %ld\n", (long) new_pos);
    self->io_offset = new_pos;
  }
  NaClLog(4, "io_offset 0x%"NACL_PRIxS", shm_written 0x%"NACL_PRIxS"\n",
          self->io_offset, self->shm_written);
  return new_pos;
}

static int NaClGioShmUnboundedFlush(struct Gio *vself) {
  struct NaClGioShmUnbounded *self = (struct NaClGioShmUnbounded *) vself;

  return (*self->ngsp->base.vtbl->Flush)(&self->ngsp->base);
}

static int NaClGioShmUnboundedClose(struct Gio *vself) {
  struct NaClGioShmUnbounded *self = (struct NaClGioShmUnbounded *) vself;

  if (NULL != self->ngsp) {
    if (0 != (*self->ngsp->base.vtbl->Close)(&self->ngsp->base)) {
      errno = EIO;
      return -1;
    }
    (*self->ngsp->base.vtbl->Dtor)(&self->ngsp->base);
    self->ngsp = NULL;
  }
  return 0;
}

static void NaClGioShmUnboundedDtor(struct Gio *vself) {
  struct NaClGioShmUnbounded *self = (struct NaClGioShmUnbounded *) vself;

  if (NULL != self->ngsp) {
    if (-1 == (*vself->vtbl->Close)(vself)) {
      NaClLog(LOG_ERROR, "NaClGioShmUnboundedDtor: auto Close failed\n");
    }
  }
  self->base.vtbl = NULL;
}

const struct GioVtbl kNaClGioShmUnboundedVtbl = {
  NaClGioShmUnboundedRead,
  NaClGioShmUnboundedWrite,
  NaClGioShmUnboundedSeek,
  NaClGioShmUnboundedFlush,
  NaClGioShmUnboundedClose,
  NaClGioShmUnboundedDtor,
};

int NaClGioShmUnboundedCtor(struct NaClGioShmUnbounded *self) {
  self->base.vtbl = NULL;
  self->ngsp = malloc(sizeof *self->ngsp);
  if (NULL == self->ngsp) {
    return 0;
  }
  if (!NaClGioShmAllocCtor(self->ngsp, NACL_MAP_PAGESIZE)) {
    free(self->ngsp);
    return 0;
  }
  self->shm_avail_sz = NACL_MAP_PAGESIZE;
  self->shm_written = 0;
  self->io_offset = 0;
  self->base.vtbl = &kNaClGioShmUnboundedVtbl;
  return 1;
}

struct NaClDesc *NaClGioShmUnboundedGetNaClDesc(
    struct NaClGioShmUnbounded  *self,
    size_t                      *written) {
  *written = self->shm_written;
  return self->ngsp->shmp;
}
