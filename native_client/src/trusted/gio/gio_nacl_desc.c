/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include "native_client/src/include/portability.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"

#include "native_client/src/trusted/gio/gio_nacl_desc.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"

const struct GioVtbl kNaClGioNaClDescVtbl;

int NaClGioNaClDescCtor(struct NaClGioNaClDesc  *self,
                        struct NaClDesc         *wrapped) {
  self->wrapped = NaClDescRef(wrapped);
  NACL_VTBL(Gio, self) = &kNaClGioNaClDescVtbl;
  return 1;
}

static ssize_t NaClGioNaClDescRead(struct Gio *vself,
                                   void       *buf,
                                   size_t     count) {
  struct NaClGioNaClDesc *self = (struct NaClGioNaClDesc *) vself;

  return (*NACL_VTBL(NaClDesc, self->wrapped)->
          Read)(self->wrapped, buf, count);
}

static ssize_t NaClGioNaClDescWrite(struct Gio *vself,
                                    void const *buf,
                                    size_t     count) {
  struct NaClGioNaClDesc *self = (struct NaClGioNaClDesc *) vself;

  return (*NACL_VTBL(NaClDesc, self->wrapped)->
          Write)(self->wrapped, buf, count);
}

static off_t NaClGioNaClDescSeek(struct Gio *vself,
                                 off_t      offset,
                                 int        whence) {
  struct NaClGioNaClDesc *self = (struct NaClGioNaClDesc *) vself;

  return (off_t) (*NACL_VTBL(NaClDesc, self->wrapped)->
                  Seek)(self->wrapped, (nacl_off64_t) offset, whence);
}

static int NaClGioNaClDescFlush(struct Gio *vself) {
  UNREFERENCED_PARAMETER(vself);
  return 0;
}

static int NaClGioNaClDescClose(struct Gio *vself) {
  struct NaClGioNaClDesc *self = (struct NaClGioNaClDesc *) vself;

  NaClDescUnref(self->wrapped);
  self->wrapped = 0;
  return 0;
}

static void NaClGioNaClDescDtor(struct Gio *vself) {
  struct NaClGioNaClDesc *self = (struct NaClGioNaClDesc *) vself;

  if (NULL != self->wrapped) {
    NaClDescUnref(self->wrapped);
    self->wrapped = 0;
  }
  NACL_VTBL(Gio, self) = NULL;
  /* Gio base class has no Dtor */
}

const struct GioVtbl kNaClGioNaClDescVtbl = {
  NaClGioNaClDescRead,
  NaClGioNaClDescWrite,
  NaClGioNaClDescSeek,
  NaClGioNaClDescFlush,
  NaClGioNaClDescClose,
  NaClGioNaClDescDtor,
};
