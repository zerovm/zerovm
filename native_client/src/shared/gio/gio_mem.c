/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Generic I/O interface implementation: memory buffer-based I/O.
 */
#include "native_client/src/include/portability.h"

#include <string.h>
#include <errno.h>

#include "native_client/src/shared/gio/gio.h"

/*
 * Memory file is just read/write from/to an in-memory buffer.  Once
 * the buffer is consumed, there is no refilling/flushing.
 */

#if !defined(SIZE_T_MAX)
# define SIZE_T_MAX ((size_t) -1)
#endif

struct GioVtbl const    kGioMemoryFileVtbl = {
  GioMemoryFileRead,
  GioMemoryFileWrite,
  GioMemoryFileSeek,
  GioMemoryFileFlush,
  GioMemoryFileClose,
  GioMemoryFileDtor,
};


int GioMemoryFileCtor(struct GioMemoryFile  *self,
                      char                  *buffer,
                      size_t                len) {
  self->buffer = buffer;
  self->len = len;
  self->curpos = 0;

  self->base.vtbl = &kGioMemoryFileVtbl;
  return 1;
}


ssize_t GioMemoryFileRead(struct Gio  *vself,
                          void        *buf,
                          size_t      count) {
  struct GioMemoryFile    *self = (struct GioMemoryFile *) vself;
  size_t                  remain;
  size_t                  newpos;

  /* 0 <= self->curpos && self->curpos <= self->len */
  remain = self->len - self->curpos;
  /* 0 <= remain <= self->len */
  if (count > remain) {
    count = remain;
  }
  /* 0 <= count && count <= remain */
  if (0 == count) {
    return 0;
  }
  newpos = self->curpos + count;
  /* self->curpos <= newpos && newpos <= self->len */

  memcpy(buf, self->buffer + self->curpos, count);
  self->curpos = newpos;
  return count;
}


ssize_t GioMemoryFileWrite(struct Gio *vself,
                           const void *buf,
                           size_t     count) {
  struct GioMemoryFile  *self = (struct GioMemoryFile *) vself;
  size_t                remain;
  size_t                newpos;

  /* 0 <= self->curpos && self->curpos <= self->len */
  remain = self->len - self->curpos;
  /* 0 <= remain <= self->len */
  if (count > remain) {
    count = remain;
  }
  /* 0 <= count && count <= remain */
  if (0 == count) {
    return 0;
  }
  newpos = self->curpos + count;
  /* self->curpos <= newpos && newpos <= self->len */

  memcpy(self->buffer + self->curpos, buf, count);
  self->curpos = newpos;
  /* we never extend a memory file */
  return count;
}


off_t GioMemoryFileSeek(struct Gio  *vself,
                        off_t       offset,
                        int         whence) {
  struct GioMemoryFile  *self = (struct GioMemoryFile *) vself;
  size_t                 new_pos = (size_t) -1;

  switch (whence) {
    case SEEK_SET:
      new_pos = offset;
      break;
    case SEEK_CUR:
      new_pos = self->curpos + offset;
      break;
    case SEEK_END:
      new_pos = (size_t) (self->len + offset);
      break;
    default:
      errno = EINVAL;
      break;
  }
  /**
   * on error, new_pos should be SIZE_T_MAX. On overflow it will either
   * be greater than self->len, or will have wrapped around.
   * TODO (ilewis): Detect wraparound and return an error.
   */
  if (new_pos > self->len) {
    errno = EINVAL;
    return (off_t) -1;
  }
  self->curpos = new_pos;
  return (off_t) new_pos;
}


int GioMemoryFileClose(struct Gio *vself) {
  UNREFERENCED_PARAMETER(vself);
  return 0;
}


int GioMemoryFileFlush(struct Gio   *vself) {
  UNREFERENCED_PARAMETER(vself);
  return 0;
}


void  GioMemoryFileDtor(struct Gio    *vself) {
  UNREFERENCED_PARAMETER(vself);
  return;
}
