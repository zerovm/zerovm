/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
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
 * NaCl Generic I/O interface implementation: in-memory snapshot of a file.
 */

#include "src/platform/gio.h"

struct GioVtbl const  kGioMemoryFileSnapshotVtbl = {
  GioMemoryFileRead,
  GioMemoryFileWrite,
  GioMemoryFileSeek,
  GioMemoryFileFlush,
  GioMemoryFileClose,
  GioMemoryFileSnapshotDtor,
};

int64_t GetFileSize(const char *name)
{
  struct stat fs;
  int handle = open(name, O_RDONLY);
  return fstat(handle, &fs), close(handle) ? -1 : fs.st_size;
}

int GioMemoryFileSnapshotCtor(struct GioMemoryFileSnapshot *self, char *fn)
{
  FILE *iop;
  char *buffer;
  size_t size = GetFileSize(fn);

  ((struct Gio *) self)->vtbl = NULL;
  if(size == (size_t)-1) return 0;
  iop = fopen(fn, "rb");
  buffer = g_malloc(size);

  if(fread(buffer, 1, size, iop) != size)
  {
    g_free(buffer);
    fclose(iop);
    return 0;
  }

  fclose(iop);
  GioMemoryFileCtor(&self->base, buffer, size);
  ((struct Gio *) self)->vtbl = &kGioMemoryFileSnapshotVtbl;
  return 1;
}

void GioMemoryFileSnapshotDtor(struct Gio *vself)
{
  struct GioMemoryFileSnapshot *self = (struct GioMemoryFileSnapshot *) vself;
  g_free(self->base.buffer);
  GioMemoryFileDtor(vself);
}
