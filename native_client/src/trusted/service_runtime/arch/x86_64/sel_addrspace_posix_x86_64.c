/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sys/mman.h>

#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


#define FOURGIG     (((size_t) 1) << 32)
#define GUARDSIZE   (10 * FOURGIG)
#define ALIGN_BITS  32
#define MSGWIDTH    "25"


/*
 * NaClAllocatePow2AlignedMemory is for allocating a large amount of
 * memory of mem_sz bytes that must be address aligned, so that
 * log_alignment low-order address bits must be zero.
 *
 * Returns the aligned region on success, or NULL on failure.
 */
static void *NaClAllocatePow2AlignedMemory(size_t mem_sz,
                                           size_t log_alignment) {
  uintptr_t pow2align;
  size_t    request_sz;
  void      *mem_ptr;
  uintptr_t orig_addr;
  uintptr_t rounded_addr;
  size_t    extra;

  pow2align = ((uintptr_t) 1) << log_alignment;

  request_sz = mem_sz + pow2align;

  NaClLog(4,
          "%"MSGWIDTH"s %016"NACL_PRIxS"\n",
          " Ask:",
          request_sz);

  mem_ptr = mmap((void *) 0,
           request_sz,
           PROT_NONE,
           MAP_ANONYMOUS | MAP_NORESERVE | MAP_PRIVATE,
           -1,
           (off_t) 0);
  if (MAP_FAILED == mem_ptr) {
    return NULL;
  }
  orig_addr = (uintptr_t) mem_ptr;

  NaClLog(4,
          "%"MSGWIDTH"s %016"NACL_PRIxPTR"\n",
          "orig memory at",
          orig_addr);

  rounded_addr = (orig_addr + (pow2align - 1)) & ~(pow2align - 1);
  extra = rounded_addr - orig_addr;

  if (0 != extra) {
    NaClLog(4,
            "%"MSGWIDTH"s %016"NACL_PRIxPTR", %016"NACL_PRIxS"\n",
            "Freeing front:",
            orig_addr,
            extra);
    if (-1 == munmap((void *) orig_addr, extra)) {
      perror("munmap (front)");
      NaClLog(LOG_FATAL,
              "NaClAllocatePow2AlignedMemory: munmap front failed\n");
    }
  }

  extra = pow2align - extra;
  if (0 != extra) {
    NaClLog(4,
            "%"MSGWIDTH"s %016"NACL_PRIxPTR", %016"NACL_PRIxS"\n",
            "Freeing tail:",
            rounded_addr + mem_sz,
            extra);
    if (-1 == munmap((void *) (rounded_addr + mem_sz),
         extra)) {
      perror("munmap (end)");
      NaClLog(LOG_FATAL,
              "NaClAllocatePow2AlignedMemory: munmap tail failed\n");
    }
  }
  NaClLog(4,
          "%"MSGWIDTH"s %016"NACL_PRIxPTR"\n",
          "Aligned memory:",
          rounded_addr);

  /*
   * we could also mmap again at rounded_addr w/o MAP_NORESERVE etc to
   * ensure that we have the memory, but that's better done in another
   * utility function.  the semantics here is no paging space
   * reserved, as in Windows MEM_RESERVE without MEM_COMMIT.
   */

  return (void *) rounded_addr;
}

NaClErrorCode NaClAllocateSpace(void **mem, size_t addrsp_size) {
  size_t        mem_sz = 2 * GUARDSIZE + FOURGIG;  /* 40G guard on each side */
  size_t        log_align = ALIGN_BITS;
  void          *mem_ptr;

  NaClLog(4, "NaClAllocateSpace(*, 0x%016"NACL_PRIxS" bytes).\n",
          addrsp_size);

  CHECK(addrsp_size == FOURGIG);

  errno = 0;
  mem_ptr = NaClAllocatePow2AlignedMemory(mem_sz, log_align);
  if (NULL == mem_ptr) {
    if (0 != errno) {
      perror("NaClAllocatePow2AlignedMemory");
    }
    NaClLog(LOG_WARNING, "Memory allocation failed\n");

    return LOAD_NO_MEMORY;
  }
  /*
   * The module lives in the middle FOURGIG of the allocated region --
   * we skip over an initial 40G guard.
   */
  *mem = (void *) (((char *) mem_ptr) + GUARDSIZE);
  NaClLog(4,
          "NaClAllocateSpace: addr space at 0x%016"NACL_PRIxPTR"\n",
          (uintptr_t) *mem);

  return LOAD_OK;
}
