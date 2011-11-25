/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/include/concurrency_ops.h"
#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_effector.h"
#include "native_client/src/trusted/desc/nacl_desc_effector.h"
#include "native_client/src/trusted/desc/nacl_desc_imc_shm.h"
#include "native_client/src/trusted/perf_counter/nacl_perf_counter.h"
#include "native_client/src/trusted/service_runtime/arch/sel_ldr_arch.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/mman.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_error_code.h"
#include "native_client/src/trusted/service_runtime/nacl_text.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"


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

static int BitmapIsBitSet(uint8_t *bitmap, uint32_t index) {
  return (bitmap[index / kBitsPerByte] & (1 << (index % kBitsPerByte))) != 0;
}

static void BitmapSetBit(uint8_t *bitmap, uint32_t index) {
  bitmap[index / kBitsPerByte] |= 1 << (index % kBitsPerByte);
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

  /*
   * Unix allows us to map pages with PROT_NONE initially and later
   * increase the mapping permissions with mprotect().
   *
   * Windows does not allow this, however: the initial permissions are
   * an upper bound on what the permissions may later be changed to
   * with VirtualProtect().  Given this, using PROT_NONE at this point
   * does not even make sense.  So we map with read+exec and
   * immediately turn down the permissions, so that we can later
   * re-enable read+exec page by page.
   */
#if NACL_WINDOWS
  mmap_protections = NACL_ABI_PROT_READ | NACL_ABI_PROT_EXEC;
#else
  mmap_protections = NACL_ABI_PROT_NONE;
#endif
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

#if NACL_WINDOWS
  {
    /*
     * We need a loop here because the Map() call above creates one
     * mapping per page.  However, there is no need for it to do that
     * for the dynamic code area.
     * TODO(mseaborn): Create a single mapping here.
     */
    uintptr_t offset;
    for (offset = 0; offset < dynamic_text_size; offset += NACL_MAP_PAGESIZE) {
      DWORD old_prot;
      if (!VirtualProtect((void *) (text_sysaddr + offset), NACL_MAP_PAGESIZE,
                          PAGE_NOACCESS, &old_prot)) {
        NaClLog(LOG_FATAL,
                "NaClMakeDynamicTextShared: VirtualProtect() failed to "
                "set page permissions to PAGE_NOACCESS\n");
      }
    }
  }
#endif

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

/*
 * Binary search nap->dynamic_regions to find the maximal region with start<=ptr
 * caller must hold nap->dynamic_load_mutex, and must discard result
 * when lock is released.
 */
struct NaClDynamicRegion* NaClDynamicRegionFindClosestLEQ(struct NaClApp *nap,
                                                          uintptr_t ptr) {
  const int kBinarySearchToScanCutoff = 16;
  int begin = 0;
  int end = nap->num_dynamic_regions;
  if (0 == nap->num_dynamic_regions) {
    return NULL;
  }
  /* as an optimization, check the last region first */
  if (nap->dynamic_regions[nap->num_dynamic_regions-1].start <= ptr) {
    return nap->dynamic_regions + nap->num_dynamic_regions-1;
  }
  /* comes before everything */
  if (ptr < nap->dynamic_regions[0].start) {
    return NULL;
  }
  /* binary search, until range is small */
  while (begin + kBinarySearchToScanCutoff + 1 < end) {
    int mid = begin + (end - begin)/2;
    if (nap->dynamic_regions[mid].start <= ptr) {
      begin = mid;
    } else {
      end = mid;
    }
  }
  /* linear scan, faster for small ranges */
  while (begin + 1 < end && nap->dynamic_regions[begin + 1].start <= ptr) {
    begin++;
  }
  return nap->dynamic_regions + begin;
}

/*
 * Find the last region overlapping with the given memory range, return 0 if
 * region is unused.
 * caller must hold nap->dynamic_load_mutex, and must discard result
 * when lock is released.
 */
struct NaClDynamicRegion* NaClDynamicRegionFind(struct NaClApp *nap,
                                                uintptr_t ptr,
                                                size_t size) {
  struct NaClDynamicRegion *p = NaClDynamicRegionFindClosestLEQ(nap,
                                                                ptr + size - 1);
  return (p != NULL && ptr < p->start + p->size) ? p : NULL;
}

/*
 * Insert a new region into nap->dynamic regions, maintaining the sorted
 * ordering. Returns 1 on success, 0 if there is a conflicting region
 * Caller must hold nap->dynamic_load_mutex.
 * Invalidates all previous NaClDynamicRegion pointers.
 */
int NaClDynamicRegionCreate(struct NaClApp *nap,
                            uintptr_t start,
                            size_t size) {
  struct NaClDynamicRegion item, *regionp, *end;
  item.start = start;
  item.size = size;
  item.delete_generation = -1;
  if (nap->dynamic_regions_allocated == nap->num_dynamic_regions) {
    /* out of space, double buffer size */
    nap->dynamic_regions_allocated *= 2;
    if (nap->dynamic_regions_allocated < kMinDynamicRegionsAllocated) {
      nap->dynamic_regions_allocated = kMinDynamicRegionsAllocated;
    }
    nap->dynamic_regions = realloc(nap->dynamic_regions,
                sizeof(struct NaClDynamicRegion) *
                   nap->dynamic_regions_allocated);
    if (NULL == nap->dynamic_regions) {
      NaClLog(LOG_FATAL, "NaClDynamicRegionCreate: realloc failed");
      return 0;
    }
  }
  /* find preceding entry */
  regionp = NaClDynamicRegionFindClosestLEQ(nap, start + size - 1);
  if (regionp != NULL && start < regionp->start + regionp->size) {
    /* target already in use */
    return 0;
  }
  if (NULL == regionp) {
    /* start at beginning if we couldn't find predecessor */
    regionp = nap->dynamic_regions;
  }
  end = nap->dynamic_regions + nap->num_dynamic_regions;
  /* scroll to insertion point (this should scroll at most 1 element) */
  for (; regionp != end && regionp->start < item.start; ++regionp);
  /* insert and shift everything forward by 1 */
  for (; regionp != end; ++regionp) {
    /* swap(*i, item); */
    struct NaClDynamicRegion t = *regionp;
    *regionp = item;
    item = t;
  }
  *regionp = item;
  nap->num_dynamic_regions++;
  return 1;
}

/*
 * Delete a region from nap->dynamic_regions, maintaining the sorted ordering
 * Caller must hold nap->dynamic_load_mutex.
 * Invalidates all previous NaClDynamicRegion pointers.
 */
void NaClDynamicRegionDelete(struct NaClApp *nap, struct NaClDynamicRegion* r) {
  struct NaClDynamicRegion *end = nap->dynamic_regions
                                + nap->num_dynamic_regions;
  /* shift everything down */
  for (; r + 1 < end; ++r) {
    r[0] = r[1];
  }
  nap->num_dynamic_regions--;

  if ( nap->dynamic_regions_allocated > kMinDynamicRegionsAllocated
     && nap->dynamic_regions_allocated/4 > nap->num_dynamic_regions) {
    /* too much waste, shrink buffer*/
    nap->dynamic_regions_allocated /= 2;
    nap->dynamic_regions = realloc(nap->dynamic_regions,
                sizeof(struct NaClDynamicRegion) *
                   nap->dynamic_regions_allocated);
    if (NULL == nap->dynamic_regions) {
      NaClLog(LOG_FATAL, "NaClDynamicRegionCreate: realloc failed");
      return;
    }
  }
}


void NaClSetThreadGeneration(struct NaClAppThread *natp, int generation) {
  /*
   * outer check handles fast case (no change)
   * since threads only set their own generation it is safe
   */
  if (natp->dynamic_delete_generation != generation)  {
    NaClXMutexLock(&natp->mu);
    CHECK(natp->dynamic_delete_generation <= generation);
    natp->dynamic_delete_generation = generation;
    NaClXMutexUnlock(&natp->mu);
  }
}

int NaClMinimumThreadGeneration(struct NaClApp *nap) {
  int i, rv = 0;
  NaClXMutexLock(&nap->threads_mu);
  for (i = 0; i < nap->num_threads; ++i) {
    struct NaClAppThread *thread = NaClGetThreadMu(nap, i);
    NaClXMutexLock(&thread->mu);
    if (i == 0 || rv > thread->dynamic_delete_generation) {
      rv = thread->dynamic_delete_generation;
    }
    NaClXMutexUnlock(&thread->mu);
  }
  NaClXMutexUnlock(&nap->threads_mu);
  return rv;
}

static void CopyBundleTails(uint8_t *dest,
                            uint8_t *src,
                            int32_t size,
                            int     bundle_size) {
  /*
   * The order in which these locations are written does not matter:
   * none of the locations will be reachable, because the bundle heads
   * still contains HLTs.
   */
  int       bundle_mask = bundle_size - 1;
  uint32_t  *src_ptr;
  uint32_t  *dest_ptr;
  uint32_t  *end_ptr;

  CHECK(0 == ((uintptr_t) dest & 3));

  src_ptr = (uint32_t *) src;
  dest_ptr = (uint32_t *) dest;
  end_ptr = (uint32_t *) (dest + size);
  while (dest_ptr < end_ptr) {
    if ((((uintptr_t) dest_ptr) & bundle_mask) != 0) {
      *dest_ptr = *src_ptr;
    }
    dest_ptr++;
    src_ptr++;
  }
}

static void CopyBundleHeads(uint8_t  *dest,
                            uint8_t  *src,
                            uint32_t size,
                            int      bundle_size) {
  /* Again, the order in which these locations are written does not matter. */
  uint8_t *src_ptr;
  uint8_t *dest_ptr;
  uint8_t *end_ptr;

  /* dest must be aligned for the writes to be atomic. */
  CHECK(0 == ((uintptr_t) dest & 3));

  src_ptr = src;
  dest_ptr = dest;
  end_ptr = dest + size;
  while (dest_ptr < end_ptr) {
    /*
     * We assume that writing the 32-bit int here is atomic, which is
     * the case on x86 and ARM as long as the address is word-aligned.
     * The read does not have to be atomic.
     */
    *(uint32_t *) dest_ptr = *(uint32_t *) src_ptr;
    dest_ptr += bundle_size;
    src_ptr += bundle_size;
  }
}

static void ReplaceBundleHeadsWithHalts(uint8_t  *dest,
                                        uint32_t size,
                                        int      bundle_size) {
  uint32_t *dest_ptr = (uint32_t*) dest;
  uint32_t *end_ptr = (uint32_t*) (dest + size);
  while (dest_ptr < end_ptr) {
    /* dont assume 1-byte halt, write entire NACL_HALT_WORD */
    *dest_ptr = NACL_HALT_WORD;
    dest_ptr += bundle_size / sizeof(uint32_t);
  }
  NaClWriteMemoryBarrier();
}

static INLINE void CopyCodeSafelyInitial(uint8_t  *dest,
                                  uint8_t  *src,
                                  uint32_t size,
                                  int      bundle_size) {
  CopyBundleTails(dest, src, size, bundle_size);
  NaClWriteMemoryBarrier();
  CopyBundleHeads(dest, src, size, bundle_size);

  /*
   * Flush the processor's instruction cache.  This is not necessary
   * for security, because any old cached instructions will just be
   * safe halt instructions.  It is only necessary to ensure that
   * untrusted code runs correctly when it tries to execute the
   * dynamically-loaded code.
   */
  NaClClearInstructionCache(dest, dest + size);
}

static void MakeDynamicCodePageVisible(struct NaClApp *nap,
                                       uint32_t page_index,
                                       uint8_t *writable_addr) {
  void *user_addr;

  if (BitmapIsBitSet(nap->dynamic_page_bitmap, page_index)) {
    /* The page is already visible: nothing to do. */
    return;
  }
  user_addr = (void *) NaClUserToSys(nap, nap->dynamic_text_start
                                     + page_index * NACL_MAP_PAGESIZE);

  /* Sanity check:  Ensure the page is not already in use. */
  CHECK(*writable_addr == 0);

  NaClFillMemoryRegionWithHalt(writable_addr, NACL_MAP_PAGESIZE);

  if (NaCl_mprotect(user_addr, NACL_MAP_PAGESIZE, PROT_READ | PROT_EXEC) != 0) {
    NaClLog(LOG_FATAL, "MakeDynamicCodePageVisible: NaCl_mprotect() failed\n");
  }

  BitmapSetBit(nap->dynamic_page_bitmap, page_index);
}

/*
 * Maps a writable version of the code at [offset, offset+size) and returns a
 * pointer to the new mapping. Internally caches the last mapping between
 * calls. Pass offset=0,size=0 to clear cache.
 * Caller must hold nap->dynamic_load_mutex.
 */
static uintptr_t CachedMapWritableText(struct NaClApp *nap,
                                       uint32_t offset,
                                       uint32_t size) {
  /*
   * The nap->* variables used in this function can be in two states:
   *
   * 1)
   * nap->dynamic_mapcache_size == 0
   * nap->dynamic_mapcache_ret == 0
   *
   * Initial state, nothing is cached.
   *
   * 2)
   * nap->dynamic_mapcache_size != 0
   * nap->dynamic_mapcache_ret != 0
   *
   * We have a cached mmap result stored, that must be unmapped.
   */
  struct NaClDescEffectorShm shm_effector;
  struct NaClDesc            *shm = nap->text_shm;
  if (!NaClDescEffectorShmCtor(&shm_effector)) {
    NaClLog(LOG_FATAL,
            "NaClTextSysDyncode_Copy: "
            "shm effector initialization failed\n");

    return -NACL_ABI_EFAULT;
  }

  if (offset != nap->dynamic_mapcache_offset
          || size != nap->dynamic_mapcache_size) {
    /*
     * cache miss, first clear the old cache if needed
     */
    if (nap->dynamic_mapcache_size > 0) {
      if (0 != (*((struct NaClDescVtbl const *) shm->base.vtbl)->
            UnmapUnsafe)(shm,
                         (struct NaClDescEffector*) &shm_effector,
                         (void*)nap->dynamic_mapcache_ret,
                         nap->dynamic_mapcache_size)) {
        NaClLog(LOG_FATAL, "CachedMapWritableText: Failed to unmap\n");
        return -NACL_ABI_EFAULT;
      }
      nap->dynamic_mapcache_offset = 0;
      nap->dynamic_mapcache_size = 0;
      nap->dynamic_mapcache_ret = 0;
    }

    /*
     * update that cached version
     */
    if (size > 0) {
      uint32_t page_index;
      uint32_t end_page_index;
      uint8_t *writable_addr;

      uintptr_t mapping = (*((struct NaClDescVtbl const *)
            shm->base.vtbl)->
              Map)(shm,
                   (struct NaClDescEffector*) &shm_effector,
                   NULL,
                   size,
                   NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
                   NACL_ABI_MAP_SHARED,
                   offset);
      if (NaClPtrIsNegErrno(&mapping)) {
        return 0;
      }

      writable_addr = (uint8_t *) mapping;
      end_page_index = (offset + size) / NACL_MAP_PAGESIZE;
      for (page_index = offset / NACL_MAP_PAGESIZE;
           page_index < end_page_index;
           page_index++) {
        MakeDynamicCodePageVisible(nap, page_index, writable_addr);
        writable_addr += NACL_MAP_PAGESIZE;
      }

      nap->dynamic_mapcache_offset = offset;
      nap->dynamic_mapcache_size = size;
      nap->dynamic_mapcache_ret = mapping;
    }
  }
  return nap->dynamic_mapcache_ret;
}

/*
 * A wrapper around CachedMapWritableText that performs common address
 * calculations.
 * Outputs *mmapped_addr.
 * Caller must hold nap->dynamic_load_mutex.
 * Returns boolean, true on success
 */
static INLINE int NaclTextMapWrapper(struct NaClApp *nap,
                                    uint32_t dest,
                                    uint32_t size,
                                    uint8_t  **mapped_addr) {
  uint32_t  shm_offset;
  uint32_t  shm_map_offset;
  uint32_t  within_page_offset;
  uint32_t  shm_map_offset_end;
  uint32_t  shm_map_size;
  uintptr_t mmap_ret;
  uint8_t   *mmap_result;

  shm_offset = dest - (uint32_t) nap->dynamic_text_start;
  shm_map_offset = shm_offset & ~(NACL_MAP_PAGESIZE - 1);
  within_page_offset = shm_offset & (NACL_MAP_PAGESIZE - 1);
  shm_map_offset_end =
    (shm_offset + size + NACL_MAP_PAGESIZE - 1) & ~(NACL_MAP_PAGESIZE - 1);
  shm_map_size = shm_map_offset_end - shm_map_offset;

  mmap_ret = CachedMapWritableText(nap,
                                   shm_map_offset,
                                   shm_map_size);
  if (0 == mmap_ret) {
    return 0;
  }
  mmap_result = (uint8_t *) mmap_ret;
  *mapped_addr = mmap_result + within_page_offset;
  return 1;
}

/*
 * Clear the mmap cache if multiple pages were mapped.
 * Caller must hold nap->dynamic_load_mutex.
 */
static INLINE void NaclTextMapClearCacheIfNeeded(struct NaClApp *nap,
                                                 uint32_t dest,
                                                 uint32_t size) {
  uint32_t                    shm_offset;
  uint32_t                    shm_map_offset;
  uint32_t                    shm_map_offset_end;
  uint32_t                    shm_map_size;
  shm_offset = dest - (uint32_t) nap->dynamic_text_start;
  shm_map_offset = shm_offset & ~(NACL_MAP_PAGESIZE - 1);
  shm_map_offset_end =
    (shm_offset + size + NACL_MAP_PAGESIZE - 1) & ~(NACL_MAP_PAGESIZE - 1);
  shm_map_size = shm_map_offset_end - shm_map_offset;
  if (shm_map_size > NACL_MAP_PAGESIZE) {
    /* call with size==offset==0 to clear cache */
    CachedMapWritableText(nap, 0, 0);
  }
}

int32_t NaClTextDyncodeCreate(struct NaClApp *nap,
                              uint32_t       dest,
                              void           *code_copy,
                              uint32_t       size) {
  uintptr_t                   dest_addr;
  uint8_t                     *mapped_addr;
  int32_t                     retval = -NACL_ABI_EINVAL;
  int                         validator_result;
  struct NaClPerfCounter      time_dyncode_create;
  NaClPerfCounterCtor(&time_dyncode_create, "NaClTextDyncodeCreate");

  if (NULL == nap->text_shm) {
    NaClLog(1, "NaClTextSysDyncode_Copy: Dynamic loading not enabled\n");
    return -NACL_ABI_EINVAL;
  }
  if (0 != (dest & (nap->bundle_size - 1)) ||
      0 != (size & (nap->bundle_size - 1))) {
    NaClLog(1, "NaClTextSysDyncode_Copy: Non-bundle-aligned address or size\n");
    return -NACL_ABI_EINVAL;
  }
  dest_addr = NaClUserToSysAddrRange(nap, dest, size);
  if (kNaClBadAddress == dest_addr) {
    NaClLog(1, "NaClTextSysDyncode_Copy: Dest address out of range\n");
    return -NACL_ABI_EFAULT;
  }
  if (dest < nap->dynamic_text_start) {
    NaClLog(1, "NaClTextSysDyncode_Copy: Below dynamic code area\n");
    return -NACL_ABI_EFAULT;
  }
  /*
   * We ensure that the final HLTs of the dynamic code region cannot
   * be overwritten, just in case of CPU bugs.
   */
  if (dest + size > nap->dynamic_text_end - NACL_HALT_SLED_SIZE) {
    NaClLog(1, "NaClTextSysDyncode_Copy: Above dynamic code area\n");
    return -NACL_ABI_EFAULT;
  }
  if (0 == size) {
    /* Nothing to load.  Succeed trivially. */
    return 0;
  }

  NaClXMutexLock(&nap->dynamic_load_mutex);

  if (NaClDynamicRegionCreate(nap, dest_addr, size) == 1) {
    /* target memory region is free */
    if (!nap->skip_validator) {
      validator_result = NaClValidateCode(nap, dest, code_copy, size);
    } else {
      NaClLog(LOG_ERROR, "VALIDATION SKIPPED.\n");
      validator_result = LOAD_OK;
    }

    NaClPerfCounterMark(&time_dyncode_create,
                        NACL_PERF_IMPORTANT_PREFIX "DynRegionValidate");
    NaClPerfCounterIntervalLast(&time_dyncode_create);
  } else {
    /* target addr is in use */
    NaClLog(1, "NaClTextSysDyncode_Copy: Code range already allocated\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup_unlock;
  }

  if (validator_result != LOAD_OK
      && nap->ignore_validator_result) {
    NaClLog(LOG_ERROR, "VALIDATION FAILED for dynamically-loaded code: "
            "continuing anyway...\n");
    validator_result = LOAD_OK;
  }

  if (validator_result != LOAD_OK) {
    NaClLog(1, "NaClTextSysDyncode_Copy: "
            "Validation of dynamic code failed\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup_unlock;
  }

  if (!NaclTextMapWrapper(nap, dest, size, &mapped_addr)) {
    retval = -NACL_ABI_ENOMEM;
    goto cleanup_unlock;
  }

  CopyCodeSafelyInitial(mapped_addr, code_copy, size, nap->bundle_size);
  retval = 0;

  NaclTextMapClearCacheIfNeeded(nap, dest, size);

 cleanup_unlock:
  NaClXMutexUnlock(&nap->dynamic_load_mutex);
  return retval;
}

int32_t NaClTextSysDyncode_Create(struct NaClAppThread *natp,
                                  uint32_t             dest,
                                  uint32_t             src,
                                  uint32_t             size) {
  struct NaClApp              *nap = natp->nap;
  uintptr_t                   src_addr;
  uint8_t                     *code_copy;
  int32_t                     retval = -NACL_ABI_EINVAL;

  src_addr = NaClUserToSysAddrRange(nap, src, size);
  if (kNaClBadAddress == src_addr) {
    NaClLog(1, "NaClTextSysDyncode_Copy: Source address out of range\n");
    return -NACL_ABI_EFAULT;
  }

  /*
   * Make a private copy of the code, so that we can validate it
   * without a TOCTTOU race condition.
   */
  code_copy = malloc(size);
  if (NULL == code_copy) {
    return -NACL_ABI_ENOMEM;
  }
  memcpy(code_copy, (uint8_t*) src_addr, size);

  retval = NaClTextDyncodeCreate(nap, dest, code_copy, size);

  free(code_copy);
  return retval;
}

int32_t NaClTextSysDyncode_Modify(struct NaClAppThread *natp,
                                  uint32_t             dest,
                                  uint32_t             src,
                                  uint32_t             size) {
  struct NaClApp              *nap = natp->nap;
  uintptr_t                   dest_addr;
  uintptr_t                   src_addr;
  uintptr_t                   beginbundle;
  uintptr_t                   endbundle;
  uintptr_t                   offset;
  uint8_t                     *mapped_addr;
  uint8_t                     *code_copy = NULL;
  uint8_t                     code_copy_buf[NACL_INSTR_BLOCK_SIZE];
  int                         validator_result;
  int32_t                     retval = -NACL_ABI_EINVAL;
  struct NaClDynamicRegion    *region;

  if (NULL == nap->text_shm) {
    NaClLog(1, "NaClTextSysDyncode_Modify: Dynamic loading not enabled\n");
    return -NACL_ABI_EINVAL;
  }

  if (0 == size) {
    /* Nothing to modify.  Succeed trivially. */
    return 0;
  }

  dest_addr = NaClUserToSysAddrRange(nap, dest, size);
  src_addr = NaClUserToSysAddrRange(nap, src, size);
  if (kNaClBadAddress == src_addr || kNaClBadAddress == dest_addr) {
    NaClLog(1, "NaClTextSysDyncode_Modify: Address out of range\n");
    return -NACL_ABI_EFAULT;
  }

  NaClXMutexLock(&nap->dynamic_load_mutex);

  region = NaClDynamicRegionFind(nap, dest_addr, size);
  if (NULL == region || region->start > dest_addr
        || region->start + region->size < dest_addr + size) {
    /* target not a subregion of region or region is null */
    NaClLog(1, "NaClTextSysDyncode_Modify: Can't find region to modify\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup_unlock;
  }

  beginbundle = dest_addr & ~(nap->bundle_size - 1);
  endbundle   = (dest_addr + size - 1 + nap->bundle_size)
                  & ~(nap->bundle_size - 1);
  offset      = dest_addr &  (nap->bundle_size - 1);
  if (endbundle-beginbundle <= sizeof code_copy_buf) {
    /* usually patches are a single bundle, so stack allocate */
    code_copy = code_copy_buf;
  } else {
    /* in general case heap allocate */
    code_copy = malloc(endbundle-beginbundle);
    if (NULL == code_copy) {
      retval = -NACL_ABI_ENOMEM;
      goto cleanup_unlock;
    }
  }

  /* copy the bundles from already-inserted code */
  memcpy(code_copy, (uint8_t*) beginbundle, endbundle - beginbundle);

  /*
   * make the requested change in temporary location
   * this avoids TOTTOU race
   */
  memcpy(code_copy + offset, (uint8_t*) src_addr, size);

  /* update dest/size to refer to entire bundles */
  dest      &= ~(nap->bundle_size - 1);
  dest_addr &= ~((uintptr_t)nap->bundle_size - 1);
  /* since both are in sandbox memory this check should succeed */
  CHECK(endbundle-beginbundle < UINT32_MAX);
  size = (uint32_t)(endbundle - beginbundle);

  /* validate this code as a replacement */
  validator_result = NaClValidateCodeReplacement(nap,
                                                 dest,
                                                 (uint8_t*) dest_addr,
                                                 code_copy,
                                                 size);

  if (validator_result != LOAD_OK
      && nap->ignore_validator_result) {
    NaClLog(LOG_ERROR, "VALIDATION FAILED for dynamically-loaded code: "
                       "continuing anyway...\n");
    validator_result = LOAD_OK;
  }

  if (validator_result != LOAD_OK) {
    NaClLog(1, "NaClTextSysDyncode_Modify: "
               "Validation of dynamic code failed\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup_unlock;
  }

  if (!NaclTextMapWrapper(nap, dest, size, &mapped_addr)) {
    retval = -NACL_ABI_ENOMEM;
    goto cleanup_unlock;
  }

  if (LOAD_OK != NaClCopyCode(nap, dest, mapped_addr, code_copy, size)) {
    NaClLog(1, "NaClTextSysDyncode_Modify "
               "Copying of replacement code failed\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup_unlock;
  }
  retval = 0;

  NaclTextMapClearCacheIfNeeded(nap, dest, size);

 cleanup_unlock:
  NaClXMutexUnlock(&nap->dynamic_load_mutex);

  if (code_copy != code_copy_buf) {
    free(code_copy);
  }

  return retval;
}

int32_t NaClTextSysDyncode_Delete(struct NaClAppThread *natp,
                                  uint32_t             dest,
                                  uint32_t             size) {
  struct NaClApp              *nap = natp->nap;
  uintptr_t                    dest_addr;
  uint8_t                     *mapped_addr;
  int32_t                     retval = -NACL_ABI_EINVAL;
  struct NaClDynamicRegion    *region;

  if (NULL == nap->text_shm) {
    NaClLog(1, "NaClTextSysDyncode_Delete: Dynamic loading not enabled\n");
    return -NACL_ABI_EINVAL;
  }

  dest_addr = NaClUserToSysAddrRange(nap, dest, size);
  if (kNaClBadAddress == dest_addr) {
    NaClLog(1, "NaClTextSysDyncode_Delete: Address out of range\n");
    return -NACL_ABI_EFAULT;
  }

  if (0 == size) {
    /* Nothing to delete.  Just update our generation. */
    int gen;
    /* fetch current generation */
    NaClXMutexLock(&nap->dynamic_load_mutex);
    gen = nap->dynamic_delete_generation;
    NaClXMutexUnlock(&nap->dynamic_load_mutex);
    /* set our generation */
    NaClSetThreadGeneration(natp, gen);
    return 0;
  }

  NaClXMutexLock(&nap->dynamic_load_mutex);

  /*
   * this check ensures the to-be-deleted region is identical to a
   * previously inserted region, so no need to check for alignment/bounds/etc
   */
  region = NaClDynamicRegionFind(nap, dest_addr, size);
  if (NULL == region || region->start != dest_addr || region->size != size) {
    NaClLog(1, "NaClTextSysDyncode_Delete: Can't find region to delete\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup_unlock;
  }


  if (region->delete_generation < 0) {
    /* first deletion request */

    if (nap->dynamic_delete_generation == INT32_MAX) {
      NaClLog(1, "NaClTextSysDyncode_Delete:"
                 "Overflow, can only delete INT32_MAX regions\n");
      retval = -NACL_ABI_EFAULT;
      goto cleanup_unlock;
    }

    if (!NaclTextMapWrapper(nap, dest, size, &mapped_addr)) {
      retval = -NACL_ABI_ENOMEM;
      goto cleanup_unlock;
    }

    /* make it so no new threads can enter target region */
    ReplaceBundleHeadsWithHalts(mapped_addr, size, nap->bundle_size);

    NaclTextMapClearCacheIfNeeded(nap, dest, size);

    /* increment and record the generation deletion was requested */
    region->delete_generation = ++nap->dynamic_delete_generation;
  }

  /* update our own generation */
  NaClSetThreadGeneration(natp, nap->dynamic_delete_generation);

  if (region->delete_generation <= NaClMinimumThreadGeneration(nap)) {
    /*
     * All threads have checked in since we marked region for deletion.
     * It is safe to remove the region.
     *
     * No need to memset the region to hlt since bundle heads are hlt
     * and thus the bodies are unreachable.
     */
    NaClDynamicRegionDelete(nap, region);
    retval = 0;
  } else {
    /*
     * Still waiting for some threads to report in...
     */
    retval = -NACL_ABI_EAGAIN;
  }

 cleanup_unlock:
  NaClXMutexUnlock(&nap->dynamic_load_mutex);
  return retval;
}
