/*
 * Copyright 2008 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL) memory object.
 */
#include "src/service_runtime/zlog.h"
#include "src/service_runtime/nacl_memory_object.h"

/*
 * Takes ownership of NaClDesc object, so no manipulation of ref count.
 */
int NaClMemObjCtor(struct NaClMemObj  *nmop,
                   struct NaClDesc    *ndp,
                   nacl_off64_t       nbytes,
                   nacl_off64_t       offset) {
  if (NULL == ndp) {
    NaClLog(LOG_FATAL, "NaClMemObjCtor: ndp is NULL\n");
  }
  nmop->ndp = ndp;
  NaClDescRef(ndp);
  nmop->nbytes = nbytes;
  nmop->offset = offset;
  return 1;
}

void NaClMemObjDtor(struct NaClMemObj *nmop) {
  NaClDescUnref(nmop->ndp);
  nmop->ndp = NULL;
  nmop->nbytes = 0;
  nmop->offset = 0;
}

void NaClMemObjSafeDtor(struct NaClMemObj *nmop) {
  if (NULL == nmop) {
    return;
  }
  NaClMemObjDtor(nmop);
}

struct NaClMemObj *NaClMemObjMake(struct NaClDesc *ndp,
                                  nacl_off64_t    nbytes,
                                  nacl_off64_t    offset) {
  struct NaClMemObj *nmop;

  if (NULL == ndp) {
    NaClLog(4, "NaClMemObjMake: invoked with NULL ndp\n");
    return NULL;  /* anonymous paging file backed memory */
  }
  if (NULL == (nmop = malloc(sizeof *nmop))) {
    NaClLog(LOG_FATAL, ("NaClMemObjMake: out of memory creating object "
                        "(NaClDesc = 0x%08"NACL_PRIxPTR", "
                        "offset = 0x%"NACL_PRIx64")\n"),
            (uintptr_t) ndp, offset);
  }
  if (!NaClMemObjCtor(nmop, ndp, nbytes, offset)) {
    NaClLog(LOG_FATAL, "NaClMemObjMake: NaClMemObjCtor failed!\n");
  }
  return nmop;
}
