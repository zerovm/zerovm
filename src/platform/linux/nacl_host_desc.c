/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.  Memory
 * mapping using descriptors.
 *
 * Note that we avoid using the thread-specific data / thread local
 * storage access to the "errno" variable, and instead use the raw
 * system call return interface of small negative numbers as errors.
 */

#include <sys/mman.h>
#include <assert.h>
#include "src/service_runtime/include/bits/mman.h"

/* for global nap and manifest using */
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/manifest/manifest_setup.h"

static INLINE int NaClMapFlagMap(int nacl_map_flags) {
  int host_os_flags;

  host_os_flags = 0;
#define M(H) do { \
    if (0 != (nacl_map_flags & NACL_ABI_ ## H)) { \
      host_os_flags |= H; \
    } \
  } while (0)
  M(MAP_SHARED);
  M(MAP_PRIVATE);
  M(MAP_FIXED);
  M(MAP_ANONYMOUS);
#undef M

  return host_os_flags;
}

/*
 * TODO(bsy): handle the !NACL_ABI_MAP_FIXED case.
 */
uintptr_t NaClHostDescMap(struct NaClHostDesc *d,
                          void                *start_addr,
                          size_t              len,
                          int                 prot,
                          int                 flags,
                          nacl_off64_t        offset) {
  int   desc;
  void  *map_addr;
  int   host_prot;
  int   host_flags;

  NaClLog(LOG_SUICIDE,
          ("NaClHostDescMap(0x%08"NACL_PRIxPTR", "
           "0x%08"NACL_PRIxPTR", 0x%08"NACL_PRIxS", "
           "0x%x, 0x%x, 0x%08"NACL_PRIx64")\n"),
          (uintptr_t) d,
          (uintptr_t) start_addr,
          len,
          prot,
          flags,
          (int64_t) offset);
  if (NULL == d && 0 == (flags & NACL_ABI_MAP_ANONYMOUS)) {
    NaClLog(LOG_FATAL, "NaClHostDescMap: 'this' is NULL and not anon map\n");
  }
  prot &= (NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE);
  /* may be PROT_NONE too, just not PROT_EXEC */

  desc = flags & NACL_ABI_MAP_ANONYMOUS ? -1 : d->d;
  /*
   * Translate flags, prot to host_flags, host_prot.
   */
  host_flags = NaClMapFlagMap(flags);
  host_prot = NaClProtMap(prot);

  NaClLog(LOG_SUICIDE, "NaClHostDescMap: host_flags 0x%x, host_prot 0x%x\n",
          host_flags, host_prot);

  /*
   * use "whole chunk" memory manager if set in manifest
   * and if this is "user side call"
   */
  assert(gnap != NULL);
  map_addr =
      gnap->user_side_flag &&
      gnap->system_manifest != NULL &&
      gnap->system_manifest->max_mem ?
          start_addr :
          mmap(start_addr, len, host_prot, host_flags, desc, offset);

  if (MAP_FAILED == map_addr) {
    NaClLog(LOG_INFO,
            ("NaClHostDescMap: "
             "mmap(0x%08"NACL_PRIxPTR", 0x%"NACL_PRIxS", "
             "0x%x, 0x%x, 0x%d, 0x%"NACL_PRIx64")"
             " failed, errno %d.\n"),
            (uintptr_t) start_addr, len, host_prot, host_flags, desc,
            (int64_t) offset,
            errno);
    return -NaClXlateErrno(errno);
  }
  if (0 != (flags & NACL_ABI_MAP_FIXED) && map_addr != start_addr) {
    NaClLog(LOG_FATAL,
            ("NaClHostDescMap: mmap with MAP_FIXED not fixed:"
             " returned 0x%08"NACL_PRIxPTR" instead of 0x%08"NACL_PRIxPTR"\n"),
            (uintptr_t) map_addr,
            (uintptr_t) start_addr);
  }
  NaClLog(LOG_SUICIDE, "NaClHostDescMap: returning 0x%08"NACL_PRIxPTR"\n",
          (uintptr_t) start_addr);

  return (uintptr_t) start_addr;
}
