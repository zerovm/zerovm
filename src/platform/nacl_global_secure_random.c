/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Secure RNG abstraction.
 */
#include "src/platform/nacl_log.h"
#include "src/platform/nacl_secure_random.h"
#include "src/platform/nacl_sync_checked.h"

static struct NaClMutex          nacl_global_rng_mu;
static struct NaClSecureRng      nacl_global_rng;
static struct NaClSecureRng      *nacl_grngp = &nacl_global_rng;

void NaClGlobalSecureRngInit(void) {
  NaClXMutexCtor(&nacl_global_rng_mu);
  if (!NaClSecureRngCtor(nacl_grngp)) {
    NaClLog(LOG_FATAL,
            "Could not construct global random number generator.\n");
  }
}

void NaClGlobalSecureRngFini(void) {
  (*nacl_grngp->base.vtbl->Dtor)(&nacl_grngp->base);
  NaClMutexDtor(&nacl_global_rng_mu);
}

uint32_t NaClGlobalSecureRngUint32(void) {
  uint32_t rv;
  NaClXMutexLock(&nacl_global_rng_mu);
  rv = (*nacl_grngp->base.vtbl->GenUint32)(&nacl_grngp->base);
  NaClXMutexUnlock(&nacl_global_rng_mu);
  return rv;
}
