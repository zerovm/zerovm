/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*
 * Copyright (c) 2012, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * NaCl Generic I/O interface implementation: memory buffer-based I/O.
 */
#include "src/platform/gio.h"

/*
 * Memory file is just read/write from/to an in-memory buffer.  Once
 * the buffer is consumed, there is no refilling/flushing.
 */
struct GioVtbl const kGioMemoryFileVtbl = {
  GioMemoryFileRead,
  GioMemoryFileWrite,
  GioMemoryFileSeek,
  GioMemoryFileFlush,
  GioMemoryFileClose,
  GioMemoryFileDtor,
};

void GioMemoryFileCtor(struct GioMemoryFile *self, char *buffer, size_t len)
{
  self->buffer = buffer;
  self->len = len;
  self->curpos = 0;

  self->base.vtbl = &kGioMemoryFileVtbl;
}

ssize_t GioMemoryFileRead(struct Gio *vself, void *buf, size_t count)
{
  struct GioMemoryFile *self = (struct GioMemoryFile *) vself;
  size_t remain;
  size_t newpos;

  /* 0 <= self->curpos && self->curpos <= self->len */
  remain = self->len - self->curpos;

  /* 0 <= remain <= self->len */
  if(count > remain) count = remain;

  /* 0 <= count && count <= remain */
  if(0 == count) return 0;

  /* self->curpos <= newpos && newpos <= self->len */
  newpos = self->curpos + count;
  memcpy(buf, self->buffer + self->curpos, count);
  self->curpos = newpos;
  return count;
}

ssize_t GioMemoryFileWrite(struct Gio *vself, const void *buf, size_t count)
{
  struct GioMemoryFile *self = (struct GioMemoryFile *) vself;
  size_t remain;
  size_t newpos;

  /* 0 <= self->curpos && self->curpos <= self->len */
  remain = self->len - self->curpos;

  /* 0 <= remain <= self->len */
  if(count > remain) count = remain;

  /* 0 <= count && count <= remain */
  if(0 == count) return 0;

  /* self->curpos <= newpos && newpos <= self->len */
  newpos = self->curpos + count;
  memcpy(self->buffer + self->curpos, buf, count);
  self->curpos = newpos;

  /* we never extend a memory file */
  return count;
}

off_t GioMemoryFileSeek(struct Gio *vself, off_t offset, int whence)
{
  struct GioMemoryFile *self = (struct GioMemoryFile *) vself;
  size_t new_pos = (size_t) -1;

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
   * TODO(ilewis): Detect wraparound and return an error.
   */
  if(new_pos > self->len)
  {
    errno = EINVAL;
    return (off_t) -1;
  }
  self->curpos = new_pos;
  return (off_t) new_pos;
}

int GioMemoryFileClose(struct Gio *vself)
{
  UNREFERENCED_PARAMETER(vself);
  return 0;
}

int GioMemoryFileFlush(struct Gio *vself)
{
  UNREFERENCED_PARAMETER(vself);
  return 0;
}

void GioMemoryFileDtor(struct Gio *vself)
{
  UNREFERENCED_PARAMETER(vself);
  return;
}
