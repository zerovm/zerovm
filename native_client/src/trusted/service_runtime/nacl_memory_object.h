/*
 * Copyright 2008 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL) memory object.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_MEMORY_OBJECT_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_MEMORY_OBJECT_H_

#include "native_client/src/include/portability.h"

#include "native_client/src/shared/imc/nacl_imc_c.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"

EXTERN_C_BEGIN

/*
 * Memory object for the virtual memory map.  We map in 64KB chunks,
 * and we need this so that we can recreate the memory from the
 * original source should we need to move the address space.
 *
 * This also means that once a descriptor / handle is used to map in
 * memory, we cannot close it unless we know how to recreate it.
 * Thus, the NaClDesc objects are reference counted.
 *
 * Any splitting of NaClMemObj objects should be done with the VM map
 * lock held, so these NaClMemObj objects do not themselves contain a
 * lock.
 */

struct NaClMemObj {
  struct NaClDesc *ndp;
  /*
   * The size of the memory object when the mapping took place.  This
   * is important becaues the last page in the mapping has to be
   * handled especially:
   *
   * If the object ends in the middle of an allocation, the kernel
   * will zero pad the allocation.  The zero bytes are writable, but
   * are not reflected back into the file.  First, assume that the
   * file size does not change.  This means that for a shared mapping,
   * when an address space move occurs we have to re-map the file at
   * the new position, and then copy the contents of the rest of the
   * allocation to the new location.
   *
   * If the file has shrunk, then the shared portion between the new
   * end of file and the original end of file is impossible to
   * duplicate as a shared mapping -- we may need to just crash and
   * burn at that point.
   *
   * If the file has grown, then the unshared portion between the old
   * end-of-file and end of that page (assuming it has grown past the
   * page boundary) probably should not get overwritten -- and we may
   * need to crash and burn at this point as well.
   *
   * On linux, we could use mremap, but then there's no need: the race
   * condition with not being able to atomically update the memory
   * mappings doesn't exist.
   *
   * TODO(bsy): figure out exactly what we need to do.
   */
  nacl_off64_t           nbytes;  /* stat member is off_t st_size */
  /*
   * All memory objects that back virtual memory must support
   * offset/nbytes mapping.  NB: the VM map code may split a
   * NaClMemObj into two if some middle page got unmapped, etc.
   */
  nacl_off64_t           offset;
};

/*
 * base class ctor and dtor.  NULL != ndp must hold.
 */
int NaClMemObjCtor(struct NaClMemObj  *nmop,
                   struct NaClDesc    *ndp,
                   nacl_off64_t       nbytes,
                   nacl_off64_t       offset) NACL_WUR;

int NaClMemObjCopyCtorOff(struct NaClMemObj *nmop,
                          struct NaClMemObj *src,
                          nacl_off64_t      additional) NACL_WUR;

void NaClMemObjDtor(struct NaClMemObj *nmop);

void NaClMemObjSafeDtor(struct NaClMemObj *nmop);

/*
 * Allocating Ctor.  Increments refcount on ndp, which must not be NULL.
 */
struct NaClMemObj *NaClMemObjMake(struct NaClDesc *ndp,
                                  nacl_off64_t    nbytes,
                                  nacl_off64_t    offset) NACL_WUR;

struct NaClMemObj *NaClMemObjSplit(struct NaClMemObj  *nmop,
                                   nacl_off64_t       additional) NACL_WUR;

void NaClMemObjIncOffset(struct NaClMemObj  *nmop,
                         nacl_off64_t       additional);

EXTERN_C_END

#endif
