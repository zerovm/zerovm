/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service runtime.  NaClDescConnCap subclass of NaClDesc.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_CONN_CAP_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_CONN_CAP_H_

#include "src/desc/nacl_desc_base.h"

EXTERN_C_BEGIN

struct NaClDescEffector;
struct NaClDescXferState;

/*
 * IMC socket addresses.  There are two variants:
 *  1) Those based on address strings.
 *  2) Those based on socket file descriptors.
 */

struct NaClDescConnCap {
  struct NaClDesc           base NACL_IS_REFCOUNT_SUBCLASS;
  struct NaClSocketAddress  cap;
};

struct NaClDescConnCapFd {
  struct NaClDesc base NACL_IS_REFCOUNT_SUBCLASS;
  NaClHandle      connect_fd;
};

int NaClDescConnCapInternalize(struct NaClDesc          **baseptr,
                               struct NaClDescXferState *xfer)
    NACL_WUR;

int NaClDescConnCapFdInternalize(struct NaClDesc          **baseptr,
                                 struct NaClDescXferState *xfer)
    NACL_WUR;

int NaClDescConnCapCtor(struct NaClDescConnCap          *self,
                        struct NaClSocketAddress const  *nsap)
    NACL_WUR;

int NaClDescConnCapFdCtor(struct NaClDescConnCapFd  *self,
                          NaClHandle                endpt)
    NACL_WUR;

EXTERN_C_END

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_CONN_CAP_H_
