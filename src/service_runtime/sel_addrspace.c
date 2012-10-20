/*
 * Copyright 2008 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */
#include <errno.h>
#include "src/include/nacl_platform.h"
#include "src/service_runtime/sel_addrspace.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/sel_memory.h"

NaClErrorCode NaClAllocAddrSpace(struct NaClApp *nap) {
  void        *mem;
  int         rv;
  uintptr_t   hole_start;
  size_t      hole_size;
  uintptr_t   stack_start;

  ZLOGS(LOG_DEBUG, "NaClAllocAddrSpace: calling NaClAllocateSpace(*,0x%016x)",
      ((size_t)1 << nap->addr_bits));

  rv = NaClAllocateSpace(&mem, (uintptr_t) 1U << nap->addr_bits);
  if (LOAD_OK != rv) return rv;

  nap->mem_start = (uintptr_t) mem;
  ZLOGS(LOG_DEBUG, "allocated memory at 0x%08x", nap->mem_start);

  hole_start = NaClRoundAllocPage(nap->data_end);

  ZLOGFAIL(nap->stack_size >= ((uintptr_t) 1U) << nap->addr_bits,
      EFAULT, "NaClAllocAddrSpace: stack too large!");
  stack_start = (((uintptr_t) 1U) << nap->addr_bits) - nap->stack_size;
  stack_start = NaClTruncAllocPage(stack_start);

  if (stack_start < hole_start) return LOAD_DATA_OVERLAPS_STACK_SECTION;

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

    if(0 != NaCl_madvise((void*)(nap->mem_start + hole_start), hole_size,  MADV_DONTNEED))
    {
      ZLOG(LOG_ERROR, "madvise, errno %d", errno);
      return LOAD_MADVISE_FAIL;
    }
    ZLOGS(LOG_DEBUG, "mprotecting 0x%08x, 0x%08x, PROT_NONE",
        nap->mem_start + hole_start, hole_size);

    if(0 != NaCl_mprotect((void *)(nap->mem_start + hole_start), hole_size, PROT_NONE))
    {
      ZLOG(LOG_ERROR, "mprotect, errno %d", errno);
      return LOAD_MPROTECT_FAIL;
    }
  }
  else
    ZLOGS(LOG_DEBUG,
      "NaClAllocAddrSpace: hole between end of data and the beginning of stack is zero size");

  return LOAD_OK;
}

/*
 * Apply memory protection to memory regions.
 */
NaClErrorCode NaClMemoryProtection(struct NaClApp *nap) {
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
  err = NaClMprotectGuards(nap);
  if (err != LOAD_OK) return err;

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
  if(0 != err)
  {
    ZLOG(LOG_ERROR, "NaClMemoryProtection: NaCl_mprotect(0x%08x, 0x%08x, 0x%x) failed, "
        "error %d (trampoline)", start_addr, region_size, PROT_READ | PROT_EXEC, err);
    return LOAD_MPROTECT_FAIL;
  }

  if(!NaClVmmapAdd(&nap->mem_map, NaClSysToUser(nap, start_addr) >> NACL_PAGESHIFT,
      region_size >> NACL_PAGESHIFT,  PROT_READ | PROT_EXEC, NULL))
  {
    ZLOG(LOG_ERROR, "NaClMemoryProtection: NaClVmmapAdd failed (trampoline)");
    return LOAD_MPROTECT_FAIL;
  }

  start_addr = NaClUserToSys(nap, nap->dynamic_text_start);
  region_size = nap->dynamic_text_end - nap->dynamic_text_start;
  ZLOGS(LOG_DEBUG, "shm txt region start 0x%08x, size 0x%08x, end 0x%08x",
          start_addr, region_size, start_addr + region_size);

  if(0 != region_size)
  {
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
    /* the log bellow added to prevent dynamic nexe loading w/o dynamic text support */
    ZLOGFAIL(1, EFAULT, "dynamic text detected. the protection should be set!");

    if(!NaClVmmapAdd(&nap->mem_map, NaClSysToUser(nap, start_addr) >> NACL_PAGESHIFT,
        region_size >> NACL_PAGESHIFT, PROT_READ | PROT_EXEC,
        NaClMemObjMake(nap->text_shm, region_size, 0)))
    {
      ZLOG(LOG_ERROR, "NaClMemoryProtection: NaClVmmapAdd failed (data)");
      return LOAD_MPROTECT_FAIL;
    }
  }

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
    if(0 != err)
    {
      ZLOG(LOG_ERROR, "NaClMemoryProtection: NaCl_mprotect(0x%08x, 0x%08x, 0x%x) "
          "failed, error %d (data)", start_addr, region_size, PROT_READ, err);
      return LOAD_MPROTECT_FAIL;
    }

    if(!NaClVmmapAdd(&nap->mem_map, NaClSysToUser(nap, start_addr) >> NACL_PAGESHIFT,
        region_size >> NACL_PAGESHIFT, PROT_READ, (struct NaClMemObj *)NULL))
    {
      ZLOG(LOG_ERROR, "NaClMemoryProtection: NaClVmmapAdd failed (data)");
      return LOAD_MPROTECT_FAIL;
    }
  }

  /*
   * data_end is max virtual addr seen, so start_addr <= data_end
   * must hold.
   */
  if(0 != nap->data_start)
  {
    start_addr = NaClUserToSys(nap, nap->data_start);
    region_size = NaClRoundPage(
        NaClRoundAllocPage(nap->data_end) - NaClSysToUser(nap, start_addr));
    ZLOGS(LOG_DEBUG, "RW data region start 0x%08x, size 0x%08x, end 0x%08x",
        start_addr, region_size, start_addr + region_size);

    if(0 != (err = NaCl_mprotect((void *)start_addr, region_size, PROT_READ | PROT_WRITE)))
    {
      ZLOG(LOG_ERROR, "NaClMemoryProtection: "
          "NaCl_mprotect(0x%08x, 0x%08lx, 0x%x) failed, error %d (data)",
          start_addr, region_size, PROT_READ | PROT_WRITE, err);
      return LOAD_MPROTECT_FAIL;
    }

    if(!NaClVmmapAdd(&nap->mem_map, NaClSysToUser(nap, start_addr) >> NACL_PAGESHIFT,
        region_size >> NACL_PAGESHIFT, PROT_READ | PROT_WRITE, (struct NaClMemObj *)NULL))
    {
      ZLOG(LOG_ERROR, "NaClMemoryProtection: NaClVmmapAdd failed (data)");
      return LOAD_MPROTECT_FAIL;
    }
  }

  /* stack is read/write but not execute */
  region_size = nap->stack_size;
  start_addr = NaClUserToSys(nap,
      NaClTruncAllocPage(((uintptr_t) 1U << nap->addr_bits) - nap->stack_size));
  ZLOGS(LOG_DEBUG, "RW stack region start 0x%08x, size 0x%08lx, end 0x%08x",
          start_addr, region_size, start_addr + region_size);

  err = NaCl_mprotect((void *)start_addr,
      NaClRoundAllocPage(nap->stack_size), PROT_READ | PROT_WRITE);
  if(0 != err)
  {
    ZLOG(LOG_ERROR, "NaClMemoryProtection: NaCl_mprotect(0x%08lx, ""0x%08lx, 0x%x) "
        "failed, error %d (stack)", start_addr, region_size, PROT_READ | PROT_WRITE, err);
    return LOAD_MPROTECT_FAIL;
  }

  if(!NaClVmmapAdd(&nap->mem_map, NaClSysToUser(nap, start_addr) >> NACL_PAGESHIFT,
      nap->stack_size >> NACL_PAGESHIFT, PROT_READ | PROT_WRITE, (struct NaClMemObj *)NULL))
  {
    ZLOG(LOG_ERROR, "NaClMemoryProtection: NaClVmmapAdd failed (stack)");
    return LOAD_MPROTECT_FAIL;
  }
  return LOAD_OK;
}
