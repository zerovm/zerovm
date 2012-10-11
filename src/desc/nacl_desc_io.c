/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.  Memory
 * mapping using descriptors.
 */

#include "src/desc/nacl_desc_effector.h"
#include "src/desc/nacl_desc_io.h"
#include "src/platform/nacl_find_addrsp.h"
#include "src/platform/nacl_log.h"
#include "src/service_runtime/include/sys/errno.h"
#include "src/service_runtime/include/sys/mman.h"
#include "src/service_runtime/internal_errno.h"
#include "src/service_runtime/nacl_config.h"

/*
 * This file contains the implementation for the NaClIoDesc subclass
 * of NaClDesc.
 *
 * NaClDescIoDesc is the subclass that wraps host-OS descriptors
 * provided by NaClHostDesc (which gives an OS-independent abstraction
 * for host-OS descriptors).
 */

static uintptr_t NaClDescIoDescMap(struct NaClDesc         *vself,
                                   struct NaClDescEffector *effp,
                                   void                    *start_addr,
                                   size_t                  len,
                                   int                     prot,
                                   int                     flags,
                                   nacl_off64_t            offset) {
  struct NaClDescIoDesc *self = (struct NaClDescIoDesc *) vself;
  int                   rv;
  uintptr_t             status;
  uintptr_t             addr;
  uintptr_t             end_addr;
  nacl_off64_t          tmp_off;

  /*
   * prot must be PROT_NONE or a combination of PROT_{READ|WRITE}
   */
  if (0 != (~(NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE) & prot)) {
    NaClLog(LOG_INFO,
            ("NaClDescIoDescMap: prot has other bits"
             " than PROT_{READ|WRITE}\n"));
    return -NACL_ABI_EINVAL;
  }

  if (0 == (NACL_ABI_MAP_FIXED & flags) && NULL == start_addr) {
    NaClLog(LOG_INFO,
            ("NaClDescIoDescMap: Mapping not NACL_ABI_MAP_FIXED"
             " but start_addr is NULL\n"));
  }

  if (0 == (NACL_ABI_MAP_FIXED & flags)) {
    if (!NaClFindAddressSpace(&addr, len)) {
      NaClLog(1, "NaClDescIoDescMap: no address space?\n");
      return -NACL_ABI_ENOMEM;
    }
    start_addr = (void *) addr;
  }
  flags |= NACL_ABI_MAP_FIXED;

  for (addr = (uintptr_t) start_addr,
           end_addr = addr + len,
           tmp_off = offset;
       addr < end_addr;
       addr += NACL_MAP_PAGESIZE,
           tmp_off += NACL_MAP_PAGESIZE) {
    size_t map_size;

    if (0 != (rv = (*effp->vtbl->UnmapMemory)(effp,
                                              addr,
                                              NACL_MAP_PAGESIZE))) {
      NaClLog(LOG_FATAL,
              ("NaClDescIoDescMap: error %d --"
               " could not unmap 0x%08"NACL_PRIxPTR
               ", length 0x%"NACL_PRIxS"\n"),
              rv,
              addr,
              (size_t) NACL_MAP_PAGESIZE);
    }

    map_size = end_addr - addr;
    if (map_size > NACL_MAP_PAGESIZE) {
      map_size = NACL_MAP_PAGESIZE;
    }
    status = NaClHostDescMap((NULL == self) ? NULL : self->hd,
                             (void *) addr,
                             map_size,
                             prot,
                             flags,
                             tmp_off);
    if (NACL_MAP_FAILED == (void *) status) {
      return -NACL_ABI_E_MOVE_ADDRESS_SPACE;
    }
  }
  return (uintptr_t) start_addr;
}

// ### can be removed since there are no customers for this service
uintptr_t NaClDescIoDescMapAnon(struct NaClDescEffector *effp,
                                void                    *start_addr,
                                size_t                  len,
                                int                     prot,
                                int                     flags,
                                nacl_off64_t            offset) {
  return NaClDescIoDescMap((struct NaClDesc *) NULL, effp, start_addr, len,
                           prot, flags, offset);
}
