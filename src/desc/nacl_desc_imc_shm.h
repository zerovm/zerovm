/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service runtime.  Transferrable shared memory objects.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_IMC_SHM_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_IMC_SHM_H_

#include "include/nacl_base.h"
#include "src/desc/nacl_desc_base.h"

EXTERN_C_BEGIN

struct NaClDesc;
struct NaClDescEffector;
struct NaClDescImcShm;
struct NaClDescXferState;
struct NaClHostDesc;
struct NaClMessageHeader;
struct nacl_abi_stat;

struct NaClDescImcShm {
  struct NaClDesc           base NACL_IS_REFCOUNT_SUBCLASS;
  NaClHandle                h;
  nacl_off64_t              size;
  /* note nacl_off64_t so struct stat incompatible */
};

int NaClDescImcShmInternalize(struct NaClDesc          **baseptr,
                              struct NaClDescXferState *xfer)
    NACL_WUR;

/*
 * Constructor: initialize the NaClDescImcShm object based on the
 * underlying low-level IMC handle h which has the given size.
 */
int NaClDescImcShmCtor(struct NaClDescImcShm  *self,
                       NaClHandle             h,
                       nacl_off64_t           size)
    NACL_WUR;

/*
 * A convenience wrapper for above, where the shm object of a given
 * size is allocated first.
 */
int NaClDescImcShmAllocCtor(struct NaClDescImcShm  *self,
                            nacl_off64_t           size,
                            int                    executable)
    NACL_WUR;

struct NaClDescImcShm *NaClDescImcShmMake(struct NaClHostDesc *nhdp)
    NACL_WUR;

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_IMC_SHM_H_
