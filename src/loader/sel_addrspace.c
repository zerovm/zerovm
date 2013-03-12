/*
 * Copyright 2008 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */
/*
 * Copyright (c) 2012, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */
#include <errno.h>
#include <sys/mman.h>
#include "src/loader/sel_addrspace.h"
#include "src/loader/sel_ldr.h"
#include "src/platform/sel_memory.h"

/* protect bumpers (guarding space) */
static void NaClMprotectGuards(struct NaClApp *nap)
{
  int err;

  ZLOGS(LOG_DEBUG, "protecting bumpers");

  /*
   * make bumpers (guard pages) with "inaccessible" protection. the "left"
   * bumper size is 40gb + 64kb, the "right" one - 40gb
   */
  err = NaCl_mprotect((void *)(nap->mem_start - GUARDSIZE),
      GUARDSIZE + NACL_SYSCALL_START_ADDR, PROT_NONE);
  ZLOGFAIL(err != 0, err, FAILED_MSG);
  err = NaCl_mprotect((void *)(nap->mem_start + FOURGIG), GUARDSIZE, PROT_NONE);
  ZLOGFAIL(err != 0, err, FAILED_MSG);

  /* put information to the memory map */
  SET_MEM_MAP_IDX(nap->mem_map[LeftBumperIdx], "LeftBumper",
      nap->mem_start - GUARDSIZE, GUARDSIZE + NACL_SYSCALL_START_ADDR, PROT_NONE);
  SET_MEM_MAP_IDX(nap->mem_map[RightBumperIdx], "RightBumper",
      nap->mem_start + FOURGIG, GUARDSIZE, PROT_NONE);
}

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

/*
 * Platform-specific routine to allocate memory space for the NaCl
 * module.  mem is an out argument; addrsp_size is the requested
 * address space size, currently always ((size_t) 1) <<
 * nap->addr_bits.  On x86-64, there's a further requirement that this
 * is 4G.
 *
 * The actual amount of memory allocated is larger than requested on
 * x86-64 and on the ARM, since guard pages are also allocated to be
 * contiguous with the allocated address space.
 *
 * If successful, the guard pages are not yet memory protected.  The
 * function NaClMprotectGuards must be called for the guard pages to
 * be active.
 *
 * update: abort zvm if failed
 */
static void NaClAllocateSpace(void **mem, size_t addrsp_size)
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

void NaClAllocAddrSpace(struct NaClApp *nap)
{
  void        *mem;
  uintptr_t   hole_start;
  size_t      hole_size;
  uintptr_t   stack_start;

  ZLOGS(LOG_DEBUG, "calling NaClAllocateSpace(*,0x%016x)", ((size_t)1 << nap->addr_bits));
  NaClAllocateSpace(&mem, (uintptr_t) 1U << nap->addr_bits);

  nap->mem_start = (uintptr_t) mem;
  ZLOGS(LOG_DEBUG, "allocated memory at 0x%08x", nap->mem_start);

  hole_start = NaClRoundAllocPage(nap->data_end);

  ZLOGFAIL(nap->stack_size >= ((uintptr_t) 1U) << nap->addr_bits,
      EFAULT, "NaClAllocAddrSpace: stack too large!");
  stack_start = (((uintptr_t) 1U) << nap->addr_bits) - nap->stack_size;
  stack_start = NaClTruncAllocPage(stack_start);

  ZLOGFAIL(stack_start < hole_start, EFAULT,
      "Memory 'hole' between end of BSS and start of stack is negative in size");

  hole_size = stack_start - hole_start;
  hole_size = NaClTruncAllocPage(hole_size);

  /*
   * mprotect and madvise unused data space to "free" it up, but
   * retain mapping so no other memory can be mapped into those
   * addresses.
   */
  if(hole_size != 0)
  {
    ZLOGS(LOG_DEBUG, "madvising 0x%08x, 0x%08x, MADV_DONTNEED",
        nap->mem_start + hole_start, hole_size);

    ZLOGFAIL(0 != NaCl_madvise((void*)(nap->mem_start + hole_start), hole_size,
        MADV_DONTNEED), errno, "madvise failed. cannot release unused data segment");

    ZLOGS(LOG_DEBUG, "mprotecting 0x%08x, 0x%08x, PROT_NONE",
        nap->mem_start + hole_start, hole_size);

    ZLOGFAIL(0 != NaCl_mprotect((void *)(nap->mem_start + hole_start), hole_size,
        PROT_NONE), errno, "mprotect failed. cannot protect pages");
  }
  else
    ZLOGS(LOG_DEBUG, "there is no hole between end of data and the beginning of stack");
}

void NaClMemoryProtection(struct NaClApp *nap)
{
  uintptr_t start_addr;
  size_t    region_size;
  int       err;

  /*
   * The first NACL_SYSCALL_START_ADDR bytes are mapped as PROT_NONE.
   * This enables NULL pointer checking, and provides additional protection
   * against addr16/data16 prefixed operations being used for attacks.
   *
   * NaClMprotectGuards also sets up guard pages outside of the
   * virtual address space of the NaClApp -- for the ARM and x86-64
   * where data sandboxing only sandbox memory writes and not reads,
   * we need to ensure that certain addressing modes that might
   * otherwise allow the NaCl app to write outside its address space
   * (given how we using masking / base registers to implement data
   * write sandboxing) won't affect the trusted data structures.
   */

  ZLOGS(LOG_DEBUG, "Protecting guard pages for 0x%08x", nap->mem_start);
  NaClMprotectGuards(nap);

  start_addr = nap->mem_start + NACL_SYSCALL_START_ADDR;
  /*
   * The next pages up to NACL_TRAMPOLINE_END are the trampolines.
   * Immediately following that is the loaded text section.
   * These are collectively marked as PROT_READ | PROT_EXEC.
   */
  region_size = NaClRoundPage(nap->static_text_end - NACL_SYSCALL_START_ADDR);
  ZLOGS(LOG_DEBUG, "Trampoline/text region start 0x%08x, size 0x%08x, end 0x%08x",
          start_addr, region_size, start_addr + region_size);

  err = NaCl_mprotect((void *)start_addr, region_size, PROT_READ | PROT_EXEC);
  ZLOGFAIL(0 != err, err, FAILED_MSG);

  SET_MEM_MAP_IDX(nap->mem_map[TextIdx], "Text",
      start_addr, region_size, PROT_READ | PROT_EXEC);

  /*
   * Page protections for this region have already been set up by
   * nacl_text.c.
   *
   * todo(d'b): since text.c exists no more, protection should be set here
   *
   * We record the mapping for consistency with other fixed
   * mappings, but the record is not actually used.  Overmapping is
   * prevented by a separate range check, which is done by
   * NaClSysCommonAddrRangeContainsExecutablePages_mu().
   */
  /*
   * zerovm does not support dynamic text. the code below will check its
   * existence, log information and fail if needed.
   * todo(d'b): after the dynamic text support will be added or completely
   * removed the block below should be rewritten or removed
   */
  start_addr = NaClUserToSys(nap, nap->dynamic_text_start);
  region_size = nap->dynamic_text_end - nap->dynamic_text_start;
  ZLOGS(LOG_DEBUG, "shm txt region start 0x%08x, size 0x%08x, end 0x%08x",
      start_addr, region_size, start_addr + region_size);
  ZLOGFAIL(0 != region_size, ENOEXEC, "zerovm does not support dynamic text");

  if(0 != nap->rodata_start)
  {
    uintptr_t rodata_end;

    /*
     * TODO(mseaborn): Could reduce the number of cases by ensuring
     * that nap->data_start is always non-zero, even if
     * nap->rodata_start == nap->data_start == nap->break_addr.
     */
    if(0 != nap->data_start)
      rodata_end = nap->data_start;
    else rodata_end = nap->break_addr;

    start_addr = NaClUserToSys(nap, nap->rodata_start);
    region_size = NaClRoundPage(NaClRoundAllocPage(rodata_end)
        - NaClSysToUser(nap, start_addr));
    ZLOGS(LOG_DEBUG, "RO data region start 0x%08x, size 0x%08x, end 0x%08x",
        start_addr, region_size, start_addr + region_size);

    err = NaCl_mprotect((void *)start_addr, region_size, PROT_READ);
    ZLOGFAIL(0 != err, err, FAILED_MSG);

    SET_MEM_MAP_IDX(nap->mem_map[RODataIdx], "ROData",
        start_addr, region_size, PROT_READ);
  }

  /*
   * data_end is max virtual addr seen, so start_addr <= data_end
   * must hold.
   */
  if(0 != nap->data_start)
  {
    start_addr = NaClUserToSys(nap, nap->data_start);
    region_size = NaClRoundPage(NaClRoundAllocPage(nap->data_end)
        - NaClSysToUser(nap, start_addr));
    ZLOGS(LOG_DEBUG, "RW data region start 0x%08x, size 0x%08x, end 0x%08x",
        start_addr, region_size, start_addr + region_size);

    err = NaCl_mprotect((void *)start_addr, region_size, PROT_READ | PROT_WRITE);
    ZLOGFAIL(0 != err, err, FAILED_MSG);

    SET_MEM_MAP_IDX(nap->mem_map[HeapIdx], "Heap",
        start_addr, region_size, PROT_READ | PROT_WRITE);
  }

  /* stack is read/write but not execute */
  region_size = nap->stack_size;
  start_addr = NaClUserToSys(nap,
      NaClTruncAllocPage(((uintptr_t) 1U << nap->addr_bits) - nap->stack_size));
  ZLOGS(LOG_DEBUG, "RW stack region start 0x%08x, size 0x%08lx, end 0x%08x",
          start_addr, region_size, start_addr + region_size);

  err = NaCl_mprotect((void *)start_addr, NaClRoundAllocPage(nap->stack_size),
      PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != err, err, FAILED_MSG);

  SET_MEM_MAP_IDX(nap->mem_map[StackIdx], "Stack",
      start_addr, NaClRoundAllocPage(nap->stack_size), PROT_READ | PROT_WRITE);
}
