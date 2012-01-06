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

#include "include/nacl_base.h"
#include "src/gio/gio_shm.h"

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

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_GIO_WRAPPED_DESC_GIO_SHM_UNBOUNDED_H_ */
