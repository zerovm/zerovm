/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/* @file
 *
 * Implementation of service runtime effector subclass used for all
 * application threads.
 */
#include "native_client/src/shared/platform/nacl_host_desc.h"

#include "native_client/src/trusted/service_runtime/nacl_desc_effector_ldr.h"

#include "native_client/src/trusted/service_runtime/include/bits/mman.h"
#include "native_client/src/trusted/service_runtime/nacl_memory_object.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_common.h"


static struct NaClDescEffectorVtbl const NaClDescEffectorLdrVtbl;  /* fwd */

int NaClDescEffectorLdrCtor(struct NaClDescEffectorLdr *self,
                            struct NaClApp             *nap) {
  self->base.vtbl = &NaClDescEffectorLdrVtbl;
  self->nap = nap;
  return 1;
}

static void NaClDescEffLdrDtor(struct NaClDescEffector *vself) {
  struct NaClDescEffectorLdr *self = (struct NaClDescEffectorLdr *) vself;

  self->nap = NULL;
  /* we did not take ownership of nap */
  vself->vtbl = (struct NaClDescEffectorVtbl const *) NULL;
}

#if NACL_WINDOWS
static int NaClDescEffLdrUnmapMemory(struct NaClDescEffector  *vself,
                                     uintptr_t                sysaddr,
                                     size_t                   nbytes) {
  struct NaClDescEffectorLdr  *self = (struct NaClDescEffectorLdr *) vself;
  uintptr_t                   addr;
  uintptr_t                   endaddr;
  uintptr_t                   usraddr;
  struct NaClVmmapEntry const *map_region;

  for (addr = sysaddr, endaddr = sysaddr + nbytes;
       addr < endaddr;
       addr += NACL_MAP_PAGESIZE) {
    usraddr = NaClSysToUser(self->nap, addr);

    map_region = NaClVmmapFindPage(&self->nap->mem_map,
                                   usraddr >> NACL_PAGESHIFT);
    if (NULL == map_region || NULL == map_region->nmop) {
      /*
       * No memory in address space, and we have only MEM_RESERVE'd
       * the address space; or memory is in address space, but not
       * backed by a file.
       */
      if (0 == VirtualFree((void *) addr, 0, MEM_RELEASE)) {
        NaClLog(LOG_FATAL,
                ("NaClMMap: VirtualFree at user addr 0x%08"NACL_PRIxPTR
                 " (sys 0x%08"NACL_PRIxPTR") failed: windows error %d\n"),
                usraddr,
                addr,
                GetLastError());
      }
    } else {
      struct NaClDesc *backing_ndp;
      int             retval;

      backing_ndp = map_region->nmop->ndp;

      retval = (*((struct NaClDescVtbl const *) backing_ndp->base.vtbl)->
                UnmapUnsafe)(backing_ndp,
                             vself,
                             (void *) addr,
                             NACL_MAP_PAGESIZE);
      if (0 != retval) {
        NaClLog(LOG_FATAL,
                ("NaClMMap: UnmapUnsafe failed at user addr 0x%08"NACL_PRIxPTR
                 " (sys 0x%08"NACL_PRIxPTR") failed: syscall return %d\n"),
                addr,
                NaClUserToSys(self->nap, addr),
                retval);
      }
    }
  }

  return 0;
}

#else  /* NACL_WINDOWS */

static int NaClDescEffLdrUnmapMemory(struct NaClDescEffector  *vself,
                                     uintptr_t                sysaddr,
                                     size_t                   nbytes) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(sysaddr);
  UNREFERENCED_PARAMETER(nbytes);
  return 0;
}
#endif  /* NACL_WINDOWS */

static uintptr_t NaClDescEffLdrMapAnonMem(struct NaClDescEffector *vself,
                                          uintptr_t               sysaddr,
                                          size_t                  nbytes,
                                          int                     prot) {
  UNREFERENCED_PARAMETER(vself);
  return NaClHostDescMap((struct NaClHostDesc *) NULL,
                         (void *) sysaddr,
                         nbytes,
                         prot,
                         (NACL_ABI_MAP_PRIVATE |
                          NACL_ABI_MAP_ANONYMOUS |
                          NACL_ABI_MAP_FIXED),
                         (off_t) 0);
}

static struct NaClDescEffectorVtbl const NaClDescEffectorLdrVtbl = {
  NaClDescEffLdrDtor,
  NaClDescEffLdrUnmapMemory,
  NaClDescEffLdrMapAnonMem,
};
