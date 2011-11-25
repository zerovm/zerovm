/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Generic I/O interface.
 */
#include "native_client/src/include/portability.h"

#include <stdlib.h>

#include "native_client/src/shared/gio/gio.h"

size_t gvprintf(struct Gio *gp,
             char const *fmt,
             va_list    ap) {
  size_t    bufsz = 1024;
  char      *buf = malloc(bufsz);
  int       rv;

  if (!buf) return -1;

  while ((rv = vsnprintf(buf, bufsz, fmt, ap)) < 0 || (unsigned) rv >= bufsz) {
    free(buf);
    buf = 0;

    /**
     * Since the buffer size wasn't big enough, we want to double it.
     * Stop doubling when we reach SIZE_MAX / 2, though, otherwise we
     * risk wraparound.
     */
    if (bufsz < SIZE_MAX / 2) {
      bufsz *= 2;
      buf = malloc(bufsz);
    }

    if (!buf) {
      return (size_t) -1;
    }
  }
  if (rv >= 0) {
    rv = (int) (*gp->vtbl->Write)(gp, buf, rv);
  }
  free(buf);

  return rv;
}


size_t gprintf(struct Gio *gp,
               char const *fmt, ...) {
  va_list ap;
  size_t  rv;

  va_start(ap, fmt);
  rv = gvprintf(gp, fmt, ap);
  va_end(ap);

  return rv;
}
