/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
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
#include <assert.h>
#include <errno.h>
#include <glib.h>
#include <sys/mman.h>
#include "src/loader/sel_ldr_x86.h"
#include "src/loader/elf_util.h"
#include "src/syscalls/switch_to_app.h"
#include "src/platform/sel_memory.h"
#include "src/loader/sel_addrspace.h"
#include "src/main/manifest_setup.h"
#include "src/main/nacl_globals.h"
#include "src/main/zlog.h"
#include "src/main/nacl_config.h"

#if !defined(SIZE_T_MAX)
# define SIZE_T_MAX     (~(size_t) 0)
#endif

/*
 * Fill from static_text_end to end of that page with halt
 * instruction, which is at least NACL_HALT_LEN in size when no
 * dynamic text is present.  Does not touch dynamic text region, which
 * should be pre-filled with HLTs.
 *
 * By adding NACL_HALT_SLED_SIZE, we ensure that the code region ends
 * with HLTs, just in case the CPU has a bug in which it fails to
 * check for running off the end of the x86 code segment.
 */
void static NaClFillEndOfTextRegion(struct NaClApp *nap) {
  size_t page_pad;

  /*
   * NOTE: make sure we are not silently overwriting data.  It is the
   * toolchain's responsibility to ensure that a NACL_HALT_SLED_SIZE
   * gap exists.
   */
  ZLOGFAIL(0 != nap->data_start && nap->static_text_end
      + NACL_HALT_SLED_SIZE > nap->data_start, EFAULT,
      "Missing gap between text and data for halt_sled");

  ZLOGFAIL(0 != nap->rodata_start && nap->static_text_end
      + NACL_HALT_SLED_SIZE > nap->rodata_start, EFAULT,
      "Missing gap between text and rodata for halt_sled");

  /* No dynamic text exists.  Space for NACL_HALT_SLED_SIZE must exist */
  page_pad = ROUNDUP_64K(nap->static_text_end + NACL_HALT_SLED_SIZE)
      - nap->static_text_end;
  ZLOGFAIL(page_pad < NACL_HALT_SLED_SIZE, EFAULT, FAILED_MSG);
  ZLOGFAIL(page_pad >= NACL_MAP_PAGESIZE + NACL_HALT_SLED_SIZE, EFAULT, FAILED_MSG);

  ZLOGS(LOG_INSANE, "Filling with halts: %08lx, %08lx bytes",
          nap->mem_start + nap->static_text_end, page_pad);

  NaClFillMemoryRegionWithHalt((void*)(nap->mem_start + nap->static_text_end), page_pad);
  nap->static_text_end += page_pad;
}

/* Basic address space layout sanity check */
static void NaClCheckAddressSpaceLayoutSanity(struct NaClApp *nap,
    uintptr_t rodata_end, uintptr_t data_end, uintptr_t max_vaddr)
{
  /* fail if Data segment exists, but is not last segment */
  if(0 != nap->data_start)
    ZLOGFAIL(data_end != max_vaddr, ENOEXEC, FAILED_MSG);
  /*
   * This should be unreachable, but we include it just for completeness
   *
   * Here is why it is unreachable:
   *
   * NaClPhdrChecks checks the test segment starting address.  The
   * only allowed loaded segments are text, data, and rodata.
   * Thus unless the rodata is in the trampoline region, it must
   * be after the text.  And NaClElfImageValidateProgramHeaders
   * ensures that all segments start after the trampoline region.
   *
   * d'b: fail if no data segment. read-only data segment exists
   * but is not last segment
   */
  else if(0 != nap->rodata_start)
    ZLOGFAIL(ROUNDUP_64K(rodata_end) != max_vaddr, ENOEXEC, FAILED_MSG);

  /* fail if Read-only data segment overlaps data segment */
  if(0 != nap->rodata_start && 0 != nap->data_start)
    ZLOGFAIL(rodata_end > nap->data_start, ENOEXEC, FAILED_MSG);

  /* fail if Text segment overlaps rodata segment */
  if(0 != nap->rodata_start)
    ZLOGFAIL(ROUNDUP_64K(NaClEndOfStaticText(nap)) > nap->rodata_start,
        ENOEXEC, FAILED_MSG);
  /* fail if No rodata segment, and text segment overlaps data segment */
  else if(0 != nap->data_start)
    ZLOGFAIL(ROUNDUP_64K(NaClEndOfStaticText(nap)) > nap->data_start,
        ENOEXEC, FAILED_MSG);

  /* fail if rodata_start not a multiple of allocation size */
  ZLOGFAIL(0 != nap->rodata_start && ROUNDUP_64K(nap->rodata_start)
    != nap->rodata_start, ENOEXEC, FAILED_MSG);

  /* fail if data_start not a multiple of allocation size */
  ZLOGFAIL(0 != nap->data_start && ROUNDUP_64K(nap->data_start)
    != nap->data_start, ENOEXEC, FAILED_MSG);
}

#define DUMP(a) ZLOGS(LOG_INSANE, "%-24s = 0x%016x", #a, a)
static void NaClLogAddressSpaceLayout(struct NaClApp *nap)
{
  ZLOGS(LOG_INSANE, "NaClApp addr space layout:");
  DUMP(nap->static_text_end);
  DUMP(nap->dynamic_text_start);
  DUMP(nap->dynamic_text_end);
  DUMP(nap->rodata_start);
  DUMP(nap->data_start);
  DUMP(nap->data_end);
  DUMP(nap->break_addr);
  DUMP(nap->initial_entry_pt);
  DUMP(nap->user_entry_pt);
  DUMP(nap->bundle_size);
}

int NaClAddrIsValidEntryPt(struct NaClApp *nap, uintptr_t addr)
{
  if(0 != (addr & (nap->bundle_size - 1))) return 0;
  return addr < nap->static_text_end;
}

void NaClAppLoadFile(struct Gio *gp, struct NaClApp *nap)
{
  uintptr_t rodata_end;
  uintptr_t data_end;
  uintptr_t max_vaddr;
  struct NaClElfImage *image = NULL;
  int err;

  /* fail if Address space too big */
  ZLOGFAIL(nap->addr_bits > NACL_MAX_ADDR_BITS, EFAULT, FAILED_MSG);

  nap->stack_size = ROUNDUP_64K(nap->stack_size);

  /* temporay object will be deleted at end of function */
  image = NaClElfImageNew(gp);
  NaClElfImageValidateElfHeader(image);

  NaClElfImageValidateProgramHeaders(image, nap->addr_bits, &nap->static_text_end,
      &nap->rodata_start, &rodata_end, &nap->data_start, &data_end, &max_vaddr);

  /*
   * if no rodata and no data, we make sure that there is space for
   * the halt sled. else if no data, but there is rodata.  this means
   * max_vaddr is just where rodata ends.  this might not be at an
   * allocation boundary, and in this the page would not be writable.
   * round max_vaddr up to the next allocation boundary so that bss
   * will be at the next writable region.
   */
  if(0 == nap->data_start)
  {
    if(0 == nap->rodata_start)
    {
      if(ROUNDUP_64K(max_vaddr) - max_vaddr < NACL_HALT_SLED_SIZE)
      {
        max_vaddr += NACL_MAP_PAGESIZE;
      }
    }
    max_vaddr = ROUNDUP_64K(max_vaddr);
  }

  /*
   * max_vaddr -- the break or the boundary between data (initialized
   * and bss) and the address space hole -- does not have to be at a
   * page boundary.
   */
  nap->break_addr = max_vaddr;
  nap->data_end = max_vaddr;

  ZLOGS(LOG_INSANE, "Values from NaClElfImageValidateProgramHeaders:");
  DUMP(nap->rodata_start);
  DUMP(rodata_end);
  DUMP(nap->data_start);
  DUMP(data_end);
  DUMP(max_vaddr);

  /* We now support only one bundle size.  */
  nap->bundle_size = NACL_INSTR_BLOCK_SIZE;

  nap->initial_entry_pt = NaClElfImageGetEntryPoint(image);
  NaClLogAddressSpaceLayout(nap);

  /* Bad program entry point address */
  ZLOGFAIL(!NaClAddrIsValidEntryPt(nap, nap->initial_entry_pt), ENOEXEC, FAILED_MSG);

  NaClCheckAddressSpaceLayoutSanity(nap, rodata_end, data_end, max_vaddr);

  ZLOGS(LOG_DEBUG, "Allocating address space");
  NaClAllocAddrSpace(nap);

  /*
   * Make sure the static image pages are marked writable before we try
   * to write them.
   */
  ZLOGS(LOG_DEBUG, "Loading into memory");
  err = NaCl_mprotect((void *)(nap->mem_start + NACL_TRAMPOLINE_START),
      ROUNDUP_64K(nap->data_end) - NACL_TRAMPOLINE_START,
      PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != err, EFAULT, "Failed to make image pages writable. code 0x%x", err);

  NaClElfImageLoad(image, gp, nap->addr_bits, nap->mem_start);

  /* d'b: shared memory for the dynamic text disabled */
  nap->dynamic_text_start = ROUNDUP_64K(NaClEndOfStaticText(nap));
  nap->dynamic_text_end = nap->dynamic_text_start;

  /*
   * NaClFillEndOfTextRegion will fill with halt instructions the
   * padding space after the static text region.
   *
   * Shm-backed dynamic text space was filled with halt instructions
   * in NaClMakeDynamicTextShared.  This extends to the rodata.  For
   * non-shm-backed text space, this extend to the next page (and not
   * allocation page).  static_text_end is updated to include the
   * padding.
   */
  NaClFillEndOfTextRegion(nap);

  ZLOGS(LOG_DEBUG, "Initializing arch switcher");
  NaClInitSwitchToApp(nap);

  ZLOGS(LOG_DEBUG, "Installing trampoline");
  NaClLoadTrampoline(nap);

  /*
   * NaClMemoryProtect also initializes the mem_map w/ information
   * about the memory pages and their current protection value.
   *
   * The contents of the dynamic text region will get remapped as
   * non-writable.
   */
  ZLOGS(LOG_DEBUG, "Applying memory protection");
  NaClMemoryProtection(nap);

  ZLOGS(LOG_DEBUG, "NaClAppLoadFile done");
  NaClLogAddressSpaceLayout(nap);

  NaClElfImageDelete(image);
}
#undef DUMP

NORETURN void CreateSession(struct NaClApp *nap)
{
  uintptr_t stack_ptr;

  assert(nap != NULL);

  /* set up user stack */
  stack_ptr = nap->mem_start + ((uintptr_t)1U << nap->addr_bits);
  stack_ptr -= STACK_USER_DATA_SIZE;
  memset((void*)stack_ptr, 0, STACK_USER_DATA_SIZE);
  ((uint32_t*)stack_ptr)[4] = 1;
  ((uint32_t*)stack_ptr)[5] = 0xfffffff0;

  /* construct "nacl_user" global */
  NaClThreadContextCtor(nacl_user, nap, nap->initial_entry_pt, stack_ptr, 0);
  nacl_user->sysret = nap->break_addr;
  nacl_user->prog_ctr = NaClUserToSys(nap, nap->initial_entry_pt);
  nacl_user->new_prog_ctr = NaClUserToSys(nap, nap->initial_entry_pt);

  /* initialize "nacl_sys" global */
  nacl_sys->rbp = NaClGetStackPtr();
  nacl_sys->rsp = NaClGetStackPtr();

  /* pass control to the nexe */
  ZLOGS(LOG_DEBUG, "SESSION STARTED");
  NaClSwitchToApp(nap, nacl_user->new_prog_ctr);
  ZLOGFAIL(1, EFAULT, "unreachable code reached");
}
