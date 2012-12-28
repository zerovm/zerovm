/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <errno.h>
#include <sys/mman.h>
#include "src/main/zlog.h"
#include "src/loader/sel_ldr.h"

/*
 * NaClAllocatePow2AlignedMemory is for allocating a large amount of
 * memory of mem_sz bytes that must be address aligned, so that
 * log_alignment low-order address bits must be zero.
 *
 * Returns the aligned region on success, or NULL on failure.
 */
static void *NaClAllocatePow2AlignedMemory(size_t mem_sz, size_t log_alignment)
{
  uintptr_t pow2align;
  size_t request_sz;
  void *mem_ptr;
  uintptr_t orig_addr;
  uintptr_t rounded_addr;
  size_t extra;

  pow2align = ((uintptr_t)1) << log_alignment;
  request_sz = mem_sz + pow2align;
  ZLOGS(LOG_INSANE, "%25s %016lx", " Ask:", request_sz);

  /* d'b: try to get the fixed address r15 (user base register) */
  /*
   * WARNING: mmap can overwrite the zerovm dynamically linked code.
   * to prevent it the code should be linked statically
   */
  mem_ptr = mmap(R15_CONST, request_sz, PROT_NONE, ABSOLUTE_MMAP, -1, (off_t)0);
  if(MAP_FAILED == mem_ptr)
  {
    ZLOG(LOG_ERROR, "the base register absolute address allocation failed!"
        " trying to allocate user space in NOT DETERMINISTIC WAY");
    mem_ptr = mmap(NULL, request_sz, PROT_NONE, RELATIVE_MMAP, -1, (off_t)0);
    ZLOGFAIL(MAP_FAILED == mem_ptr, ENOMEM, FAILED_MSG);
  }

  orig_addr = (uintptr_t)mem_ptr;
  ZLOGS(LOG_INSANE, "%25s %016lx", "orig memory at", orig_addr);

  rounded_addr = (orig_addr + (pow2align - 1)) & ~(pow2align - 1);
  extra = rounded_addr - orig_addr;
  if(0 != extra)
  {
    ZLOGS(LOG_INSANE, "%25s %016lx, %016lx", "Freeing front:", orig_addr, extra);
    ZLOGFAIL(-1 == munmap((void *)orig_addr, extra), errno, "munmap front failed");
  }

  extra = pow2align - extra;
  if(0 != extra)
  {
    ZLOGS(LOG_INSANE, "%25s %016lx, %016lx", "Freeing tail:", rounded_addr + mem_sz, extra);
    ZLOGFAIL(-1 == munmap((void *)(rounded_addr + mem_sz), extra), errno, "munmap tail failed");
  }

  ZLOGS(LOG_INSANE, "%25s %016lx", "Aligned memory:", rounded_addr);

  /*
   * we could also mmap again at rounded_addr w/o MAP_NORESERVE etc to
   * ensure that we have the memory, but that's better done in another
   * utility function.  the semantics here is no paging space
   * reserved, as in Windows MEM_RESERVE without MEM_COMMIT.
   */

  return (void *)rounded_addr;
}

void NaClAllocateSpace(void **mem, size_t addrsp_size)
{
  size_t mem_sz = 2 * GUARDSIZE + FOURGIG; /* 40G guard on each side */
  size_t log_align = ALIGN_BITS;
  void *mem_ptr;

  ZLOGS(LOG_INSANE, "NaClAllocateSpace(*, 0x%016lx bytes)", addrsp_size);
  ZLOGFAIL(addrsp_size != FOURGIG, EFAULT, "addrsp_size != FOURGIG");

  errno = 0;
  mem_ptr = NaClAllocatePow2AlignedMemory(mem_sz, log_align);
  ZLOGFAIL(NULL == mem_ptr, errno, "NaClAllocatePow2AlignedMemory failed");

  /*
   * The module lives in the middle FOURGIG of the allocated region --
   * we skip over an initial 40G guard.
   */
  *mem = (void *)(((char *)mem_ptr) + GUARDSIZE);
  ZLOGS(LOG_INSANE, "addr space at 0x%016lx", (uintptr_t)*mem);
}
