/*
 * Copyright 2008 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL) memory object.
 */
#include <errno.h>
#include "src/service_runtime/zlog.h"
#include "src/service_runtime/nacl_memory_object.h"

/*
 * Takes ownership of NaClDesc object, so no manipulation of ref count.
 */
int NaClMemObjCtor(struct NaClMemObj *nmop, struct NaClDesc *ndp, nacl_off64_t nbytes,
    nacl_off64_t offset)
{
  ZLOGFAIL(NULL == ndp, EFAULT, "NaClMemObjCtor: ndp is NULL");
  nmop->ndp = ndp;
  NaClDescRef(ndp);
  nmop->nbytes = nbytes;
  nmop->offset = offset;
  return 1;
}

void NaClMemObjDtor(struct NaClMemObj *nmop)
{
  NaClDescUnref(nmop->ndp);
  nmop->ndp = NULL;
  nmop->nbytes = 0;
  nmop->offset = 0;
}

void NaClMemObjSafeDtor(struct NaClMemObj *nmop)
{
  if(NULL == nmop) return;
  NaClMemObjDtor(nmop);
}

struct NaClMemObj *NaClMemObjMake(struct NaClDesc *ndp,
    nacl_off64_t nbytes, nacl_off64_t offset)
{
  struct NaClMemObj *nmop;

  if(NULL == ndp)
  {
    ZLOG(LOG_DEBUG, "NaClMemObjMake: invoked with NULL ndp");
    return NULL; /* anonymous paging file backed memory */
  }

  nmop = malloc(sizeof *nmop);
  ZLOGFAIL(NULL == nmop, ENOMEM, "ndp is NULL");
  ZLOGFAIL(!NaClMemObjCtor(nmop, ndp, nbytes, offset), EFAULT, "NaClMemObjCtor failed!");

  return nmop;
}
