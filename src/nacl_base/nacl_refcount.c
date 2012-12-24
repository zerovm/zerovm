/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <errno.h>
#include <glib.h>
#include "src/nacl_base/nacl_refcount.h"
#include "src/service_runtime/zlog.h"

struct NaClRefCount *NaClRefCountRef(struct NaClRefCount *nrcp)
{
  ZLOG(LOG_INSANE, "NaClRefCountRef(0x%08lx)", (uintptr_t)nrcp);
  ZLOGFAIL(0 == ++nrcp->ref_count, EFAULT, "NaClRefCountRef integer overflow");
  return nrcp;
}

void NaClRefCountUnref(struct NaClRefCount *nrcp)
{
  int destroy;

  ZLOG(LOG_INSANE, "NaClRefCountUnref(0x%08lx)", (uintptr_t)nrcp);
  ZLOGFAIL(0 == nrcp->ref_count, EFAULT,
      "NaClRefCountUnref on 0x%08ld, refcount already zero!", (uintptr_t)nrcp);

  destroy = (0 == --nrcp->ref_count);
  if(destroy)
  {
    (*nrcp->vtbl->Dtor)(nrcp);
    g_free(nrcp);
  }
}
