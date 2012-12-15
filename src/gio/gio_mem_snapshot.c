/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Generic I/O interface implementation: in-memory snapshot of a file.
 */

#include <sys/stat.h>
#include <glib.h>
#include "src/gio/gio.h"
#include "src/service_runtime/zlog.h"

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
  free(self->base.buffer);
  GioMemoryFileDtor(vself);
}
