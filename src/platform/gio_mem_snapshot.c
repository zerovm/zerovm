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

#include <sys/stat.h>
#include <glib.h>
#include "src/platform/gio.h"
#include "src/main/zlog.h"

struct GioVtbl const  kGioMemoryFileSnapshotVtbl = {
  GioMemoryFileRead,
  GioMemoryFileWrite,
  GioMemoryFileSeek,
  GioMemoryFileFlush,
  GioMemoryFileClose,
  GioMemoryFileSnapshotDtor,
};

/* todo(d'b): rewrite or (even better) remove */
int GioMemoryFileSnapshotCtor(struct GioMemoryFileSnapshot *self, char *fn)
{
  FILE *iop;
  struct stat stbuf;
  char *buffer;

  ((struct Gio *) self)->vtbl = (struct GioVtbl *) NULL;
  if(0 == (iop = fopen(fn, "rb")))
  {
    return 0;
  }

  if(fstat(fileno(iop), &stbuf) == -1)
  {
    abort0: fclose(iop);
    return 0;
  }

  buffer = g_malloc(stbuf.st_size);
  if(fread(buffer, 1, stbuf.st_size, iop) != (size_t) stbuf.st_size)
  {
    abort1: g_free(buffer);
    goto abort0;
  }

  if(GioMemoryFileCtor(&self->base, buffer, stbuf.st_size) == 0)
  {
    goto abort1;
  }

  (void) fclose(iop);
  ((struct Gio *) self)->vtbl = &kGioMemoryFileSnapshotVtbl;
  return 1;
}

void GioMemoryFileSnapshotDtor(struct Gio                     *vself) {
  struct GioMemoryFileSnapshot  *self = (struct GioMemoryFileSnapshot *)
      vself;
  g_free(self->base.buffer);
  GioMemoryFileDtor(vself);
}
