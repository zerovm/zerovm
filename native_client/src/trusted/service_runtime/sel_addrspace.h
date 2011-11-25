/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */

#ifndef __SEL_ADDRSPACE_H__
#define __SEL_ADDRSPACE_H__ 1

#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/nacl_error_code.h"

struct NaClApp; /* fwd */

NaClErrorCode NaClAllocAddrSpace(struct NaClApp *nap) NACL_WUR;

/*
 * Apply memory protection to memory regions.
 */
NaClErrorCode NaClMemoryProtection(struct NaClApp *nap) NACL_WUR;

/*
 * Platform-specific routine to allocate memory space for the NaCl
 * module.  mem is an out argument; addrsp_size is the requested
 * address space size, currently always ((size_t) 1) <<
 * nap->addr_bits.  On x86-64, there's a further requirement that this
 * is 4G.
 *
 * The actual amount of memory allocated is larger than requested on
 * x86-64 and on the ARM, since guard pages are also allocated to be
 * contiguous with the allocated address space.
 *
 * If successful, the guard pages are not yet memory protected.  The
 * function NaClMprotectGuards must be called for the guard pages to
 * be active.
 *
 * Returns LOAD_OK on success.
 */
NaClErrorCode NaClAllocateSpace(void **mem, size_t addrsp_size) NACL_WUR;

NaClErrorCode NaClMprotectGuards(struct NaClApp *nap);
#endif
