/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Generic I/O interface.
 */
#include <syslog.h>
#include "src/gio/gio.h"

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
    /*
     * d'b(LOG): if defined alternative log destination {{
     * todo(d'b): replace this patch with permanent solution
     * (remove nacl log completely)
     */
#define SYSLOG_LOG
#ifdef SYSLOG_LOG
    /* put log message to the syslog */
    int CurrentDetailLevel(); /* accessor for "current_detail_level" */
    int priority;

    switch(CurrentDetailLevel())
    {
      /* debug */
      case 0: case 1: case 2: case 3: case 4: case 5: case 6:
        priority = 7;
        break;
      /* info */
      case -1:
        priority = 6;
        break;
      /* warning */
      case -2:
        priority = 4;
        break;
      /* error */
      case -3:
        priority = 3;
        break;
      /* fatal. invokes zerovm finalization */
      case -4:
        priority = 0;
        break;
      /* unknown verbosity level treated as debug */
      default:
        priority = 7;
        break;
    }
    syslog(priority, "%s", buf);
#else
    rv = (int) (*gp->vtbl->Write)(gp, buf, rv);
#endif
    /* }} */
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
