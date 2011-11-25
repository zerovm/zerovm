/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * Subclass of the gio object for use in trusted code, providing I/O
 * services to a shared memory object.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_GIO_WRAPPED_DESC_GIO_SHM_H_
#define NATIVE_CLIENT_SRC_TRUSTED_GIO_WRAPPED_DESC_GIO_SHM_H_

#include "native_client/src/include/nacl_base.h"

#include "native_client/src/shared/gio/gio.h"

#include "native_client/src/trusted/desc/nacl_desc_effector_trusted_mem.h"

EXTERN_C_BEGIN

struct NaClDesc;
struct NaClDescEffector;

struct NaClGioShm {
  /* public */
  struct Gio                        base;
  struct NaClDesc                   *shmp;
  /*
   * the shmp is public for e.g. xferring via nrd_xfer
   */

  /* private */
  struct NaClDescEffectorTrustedMem eff;

  size_t                            io_offset;
  size_t                            shm_sz;
  char                              *cur_window;
  size_t                            window_offset;
  size_t                            window_size;
};

/*
 * Create a Gio object backed by a NaClDesc -- any NaClDesc objects
 * for which the Map virtual function is usable.  The shm_size is the
 * size of the file data in the shm object -- so the requirement is
 * that *shmp object's size is at least shm_size (and it would
 * normally be, since the object size is rounded to
 * NACL_MAP_PAGESIZE).  The shm_size value is used to limit Seek and
 * to know when to report EOF on Read.
 *
 * The NaClGioShm object is read-only.  It takes a reference to the
 * shmp when constructed, and releases a reference on destruction.
 */
int NaClGioShmCtor(struct NaClGioShm  *self,
                   struct NaClDesc    *shmp,
                   size_t             shm_size);

int NaClGioShmAllocCtor(struct NaClGioShm *self,
                        size_t            shm_size);

/* Dtor is a virtual function */

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_GIO_WRAPPED_DESC_GIO_SHM_H_ */
