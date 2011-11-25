/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * Subclass of the gio object for use in trusted code.  Provides write
 * buffering into a shared memory object that may grow as needed.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_GIO_WRAPPED_DESC_GIO_SHM_UNBOUNDED_H_
#define NATIVE_CLIENT_SRC_TRUSTED_GIO_WRAPPED_DESC_GIO_SHM_UNBOUNDED_H_

#include "native_client/src/include/nacl_base.h"

#include "native_client/src/shared/gio/gio.h"

#include "native_client/src/trusted/gio/gio_shm.h"

EXTERN_C_BEGIN

struct NaClGioShm;

struct NaClGioShmUnbounded {
  /* public */
  struct Gio        base;
  struct NaClGioShm *ngsp;

  /* private */
  size_t            shm_avail_sz;
  size_t            shm_written;
  size_t            io_offset;  /* deal with seeks */
};


/*
 * When the NaClGioShmUnbounded buffer writes are all done,
 * NaClGioShmUnboundedGetNaClDesc is used to obtain the NaClDesc
 * pointer -- caller is responsible for taking another reference with
 * NaClDescRef, if the lifetime must extend beyond that of the
 * NaClGioShmUnbounded object -- and the actual size.  Actual size is
 * the largest offset written, not number of bytes written, due to
 * Seek and the potential of overlapping Writes (or gaps).
 */
struct NaClDesc *NaClGioShmUnboundedGetNaClDesc(
    struct NaClGioShmUnbounded  *self,
    size_t                      *written);


int NaClGioShmUnboundedCtor(struct NaClGioShmUnbounded *self);


EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_GIO_WRAPPED_DESC_GIO_SHM_UNBOUNDED_H_ */
