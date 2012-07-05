/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <string.h>

#include "include/concurrency_ops.h"
#include "include/nacl_platform.h"
#include "src/platform/nacl_check.h"
#include "src/platform/nacl_sync_checked.h"
#include "src/desc/nacl_desc_effector.h"
#include "src/desc/nacl_desc_imc_shm.h"
#include "src/perf_counter/nacl_perf_counter.h"
#include "src/service_runtime/include/sys/errno.h"
#include "src/service_runtime/include/sys/mman.h"
#include "src/service_runtime/nacl_text.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/sel_memory.h"

/* initial size of the malloced buffer for dynamic regions */
static const int kMinDynamicRegionsAllocated = 32;

static const int kBitsPerByte = 8;

static uint8_t *BitmapAllocate(uint32_t indexes) {
  uint32_t byte_count = (indexes + kBitsPerByte - 1) / kBitsPerByte;
  uint8_t *bitmap = malloc(byte_count);
  if (bitmap != NULL) {
    memset(bitmap, 0, byte_count);
  }
  return bitmap;
}

/*
 * Private subclass of NaClDescEffector, used only in this file.
 */
struct NaClDescEffectorShm {
  struct NaClDescEffector   base;
};

static
void NaClDescEffectorShmDtor(struct NaClDescEffector *vself) {
  /* no base class dtor to invoke */

  vself->vtbl = (struct NaClDescEffectorVtbl *) NULL;

  return;
}

static
int NaClDescEffectorShmUnmapMemory(struct NaClDescEffector  *vself,
                                   uintptr_t                sysaddr,
                                   size_t                   nbytes) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(sysaddr);
  UNREFERENCED_PARAMETER(nbytes);
  return 0;
}

static
uintptr_t NaClDescEffectorShmMapAnonymousMemory(struct NaClDescEffector *vself,
                                                uintptr_t               sysaddr,
                                                size_t                  nbytes,
                                                int                     prot) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(sysaddr);
  UNREFERENCED_PARAMETER(nbytes);
  UNREFERENCED_PARAMETER(prot);

  NaClLog(LOG_FATAL, "NaClDescEffectorShmMapAnonymousMemory called\n");
  /* NOTREACHED but gcc doesn't know that */
  return -NACL_ABI_EINVAL;
}

static
struct NaClDescEffectorVtbl kNaClDescEffectorShmVtbl = {
  NaClDescEffectorShmDtor,
  NaClDescEffectorShmUnmapMemory,
  NaClDescEffectorShmMapAnonymousMemory,
};

int NaClDescEffectorShmCtor(struct NaClDescEffectorShm *self) {
  self->base.vtbl = &kNaClDescEffectorShmVtbl;
  return 1;
}

NaClErrorCode NaClMakeDynamicTextShared(struct NaClApp *nap) {
  enum NaClErrorCode          retval = LOAD_INTERNAL;
  uintptr_t                   dynamic_text_size;
  struct NaClDescImcShm       *shm = NULL;
  struct NaClDescEffectorShm  shm_effector;
  int                         shm_effector_initialized = 0;
  uintptr_t                   shm_vaddr_base;
  int                         mmap_protections;
  uintptr_t                   mmap_ret;

  uintptr_t                   shm_upper_bound;
  uintptr_t                   text_sysaddr;

  shm_vaddr_base = NaClEndOfStaticText(nap);
  NaClLog(4,
          "NaClMakeDynamicTextShared: shm_vaddr_base = %08"NACL_PRIxPTR"\n",
          shm_vaddr_base);
  shm_vaddr_base = NaClRoundAllocPage(shm_vaddr_base);
  NaClLog(4,
          "NaClMakeDynamicTextShared: shm_vaddr_base = %08"NACL_PRIxPTR"\n",
          shm_vaddr_base);

  if (!nap->use_shm_for_dynamic_text) {
    NaClLog(4,
            "NaClMakeDynamicTextShared:"
            "  rodata / data segments not allocation aligned\n");
    NaClLog(4,
            " not using shm for text\n");
    nap->dynamic_text_start = shm_vaddr_base;
    nap->dynamic_text_end = shm_vaddr_base;
    return LOAD_OK;
  }

  /*
   * Allocate a shm region the size of which is nap->rodata_start -
   * end-of-text.  This implies that the "core" text will not be
   * backed by shm.
   */
  shm_upper_bound = nap->rodata_start;
  if (0 == shm_upper_bound) {
    shm_upper_bound = nap->data_start;
  }
  if (0 == shm_upper_bound) {
    shm_upper_bound = shm_vaddr_base;
  }
  nap->dynamic_text_start = shm_vaddr_base;
  nap->dynamic_text_end = shm_upper_bound;

  NaClLog(4, "shm_upper_bound = %08"NACL_PRIxPTR"\n", shm_upper_bound);

  dynamic_text_size = shm_upper_bound - shm_vaddr_base;
  NaClLog(4,
          "NaClMakeDynamicTextShared: dynamic_text_size = %"NACL_PRIxPTR"\n",
          dynamic_text_size);

  if (0 == dynamic_text_size) {
    NaClLog(4, "Empty JITtable region\n");
    return LOAD_OK;
  }

  shm = (struct NaClDescImcShm *) malloc(sizeof *shm);
  if (NULL == shm) {
    goto cleanup;
  }
  if (!NaClDescImcShmAllocCtor(shm, dynamic_text_size, /* executable= */ 1)) {
    /* cleanup invariant is if ptr is non-NULL, it's fully ctor'd */
    free(shm);
    shm = NULL;
    NaClLog(4, "NaClMakeDynamicTextShared: shm creation for text failed\n");
    retval = LOAD_NO_MEMORY;
    goto cleanup;
  }
  if (!NaClDescEffectorShmCtor(&shm_effector)) {
    NaClLog(4,
            "NaClMakeDynamicTextShared: shm effector"
            " initialization failed\n");
    retval = LOAD_INTERNAL;
    goto cleanup;
  }
  shm_effector_initialized = 1;

  text_sysaddr = NaClUserToSys(nap, shm_vaddr_base);

  /* Existing memory is anonymous paging file backed. */
  NaCl_page_free((void *) text_sysaddr, dynamic_text_size);

  mmap_protections = NACL_ABI_PROT_NONE;
  NaClLog(4,
          "NaClMakeDynamicTextShared: Map(,,0x%"NACL_PRIxPTR",size = 0x%x,"
          " prot=0x%x, flags=0x%x, offset=0)\n",
          text_sysaddr,
          (int) dynamic_text_size,
          mmap_protections,
          NACL_ABI_MAP_SHARED | NACL_ABI_MAP_FIXED);
  mmap_ret = (*((struct NaClDescVtbl const *) shm->base.base.vtbl)->
              Map)((struct NaClDesc *) shm,
                   (struct NaClDescEffector *) &shm_effector,
                   (void *) text_sysaddr,
                   dynamic_text_size,
                   mmap_protections,
                   NACL_ABI_MAP_SHARED | NACL_ABI_MAP_FIXED,
                   0);
  if (text_sysaddr != mmap_ret) {
    NaClLog(LOG_FATAL, "Could not map in shm for dynamic text region\n");
  }

  nap->dynamic_page_bitmap =
    BitmapAllocate((uint32_t) (dynamic_text_size / NACL_MAP_PAGESIZE));
  if (NULL == nap->dynamic_page_bitmap) {
    NaClLog(LOG_FATAL, "NaClMakeDynamicTextShared: BitmapAllocate() failed\n");
  }

  nap->text_shm = &shm->base;
  retval = LOAD_OK;

 cleanup:
  if (shm_effector_initialized) {
    (*shm_effector.base.vtbl->Dtor)((struct NaClDescEffector *) &shm_effector);
  }
  if (LOAD_OK != retval) {
    NaClDescSafeUnref((struct NaClDesc *) shm);
    free(shm);
  }

  return retval;
}

