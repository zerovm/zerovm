/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.  Memory
 * mapping using descriptors.
 */

#include "src/desc/nacl_desc_base.h"
#include "src/platform/nacl_log.h"
#include "src/nacl_base/nacl_refcount.h"
#include "src/service_runtime/include/sys/errno.h"

/*
 * This file contains base class code for NaClDesc.
 *
 * The implementation for following subclasses are elsewhere, but here
 * is an enumeration of them with a brief description:
 *
 * NaClDescIoDesc is the subclass that wraps host-OS descriptors
 * provided by NaClHostDesc (which gives an OS-independent abstraction
 * for host-OS descriptors).
 *
 * NaClDescImcDesc is the subclass that wraps IMC descriptors.
 *
 * NaClDescMutex and NaClDescCondVar are the subclasses that
 * wrap the non-transferrable synchronization objects.
 *
 * These NaClDesc objects are impure in that they know about the
 * virtual memory subsystem restriction of requiring mappings to occur
 * in NACL_MAP_PAGESIZE (64KB) chunks, so the Map and Unmap virtual
 * functions, at least, will enforce this restriction.
 */

/*
 * TODO(bsy): remove when we put SIZE_T_MAX in a common header file.
 */
#if !defined(SIZE_T_MAX)
# define SIZE_T_MAX ((size_t) -1)
#endif

struct NaClDesc *NaClDescRef(struct NaClDesc *ndp) {
  return (struct NaClDesc *) NaClRefCountRef(&ndp->base);
}

void NaClDescUnref(struct NaClDesc *ndp) {
  NaClRefCountUnref(&ndp->base);
}

/* d'b: define changed to avoid compiler warning */
#define MAP(E) case E: do { return #E; } while (0); break
char const *NaClDescTypeString(enum NaClDescTypeTag type_tag) {
  /* default functions for the vtable - return NOT_IMPLEMENTED */
  switch (type_tag) {
    MAP(NACL_DESC_INVALID);
    MAP(NACL_DESC_DIR);
    MAP(NACL_DESC_HOST_IO);
    MAP(NACL_DESC_CONN_CAP);
    MAP(NACL_DESC_CONN_CAP_FD);
    MAP(NACL_DESC_BOUND_SOCKET);
    MAP(NACL_DESC_CONNECTED_SOCKET);
    MAP(NACL_DESC_SHM);
    MAP(NACL_DESC_SYSV_SHM);
    MAP(NACL_DESC_MUTEX);
    MAP(NACL_DESC_CONDVAR);
    MAP(NACL_DESC_SEMAPHORE);
    MAP(NACL_DESC_SYNC_SOCKET);
    MAP(NACL_DESC_TRANSFERABLE_DATA_SOCKET);
    MAP(NACL_DESC_IMC_SOCKET);
    default: return "BAD TYPE TAG";
  }
}
