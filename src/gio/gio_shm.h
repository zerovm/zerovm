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

#include "include/nacl_base.h"
#include "src/gio/gio.h"
#include "src/desc/nacl_desc_effector_trusted_mem.h"

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

int NaClGioShmAllocCtor(struct NaClGioShm *self,
                        size_t            shm_size);

/* Dtor is a virtual function */

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_GIO_WRAPPED_DESC_GIO_SHM_H_ */
