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

#include <assert.h>
#include <sys/mman.h>
#include "src/loader/elf_util.h"
#include "src/syscalls/switch_to_app.h"
#include "src/loader/userspace.h"
#include "src/loader/usermap.h"

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
void static FillEndOfTextRegion(struct NaClApp *nap)
{
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
      MEM_START + nap->static_text_end, page_pad);

  FillMemoryRegionWithHalt((void*)MEM_START + nap->static_text_end, page_pad);
  nap->static_text_end += page_pad;
}

/* Basic address space layout sanity check */
static void CheckAddressSpaceLayoutSanity(struct NaClApp *nap,
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
   * PhdrChecks checks the test segment starting address.  The
   * only allowed loaded segments are text, data, and rodata.
   * Thus unless the rodata is in the trampoline region, it must
   * be after the text.  And ValidateProgramHeaders ensures that
   * all segments start after the trampoline region.
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
static void LogAddressSpaceLayout(struct NaClApp *nap)
{
  ZLOGS(LOG_INSANE, "NaClApp addr space layout:");
  DUMP(nap->static_text_end);
  DUMP(nap->rodata_start);
  DUMP(nap->data_start);
  DUMP(nap->data_end);
  DUMP(nap->break_addr);
  DUMP(nap->initial_entry_pt);
}

static int AddrIsValidEntryPt(struct NaClApp *nap, uintptr_t addr)
{
  if(0 != (addr & (NACL_INSTR_BLOCK_SIZE - 1))) return 0;
  return addr < nap->static_text_end;
}

// ### MakeUserSpace() and SetUserSpace() calls should be extracted
void AppLoadFile(struct Gio *gp, struct NaClApp *nap)
{
  uintptr_t rodata_end;
  uintptr_t data_end;
  uintptr_t max_vaddr;
  struct ElfImage *image = NULL;
  int err;

  /* temporay object will be deleted at end of function */
  image = ElfImageNew(gp);
  ValidateElfHeader(image);

  ValidateProgramHeaders(image, &nap->static_text_end, &nap->rodata_start,
      &rodata_end, &nap->data_start, &data_end, &max_vaddr);

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
        max_vaddr += NACL_MAP_PAGESIZE;
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

  ZLOGS(LOG_INSANE, "Values from ValidateProgramHeaders:");
  DUMP(nap->rodata_start);
  DUMP(rodata_end);
  DUMP(nap->data_start);
  DUMP(data_end);
  DUMP(max_vaddr);

  nap->initial_entry_pt = ElfImageGetEntryPoint(image);
  LogAddressSpaceLayout(nap);

  /* Bad program entry point address */
  ZLOGFAIL(!AddrIsValidEntryPt(nap, nap->initial_entry_pt), ENOEXEC, FAILED_MSG);

  CheckAddressSpaceLayoutSanity(nap, rodata_end, data_end, max_vaddr);

  ZLOGS(LOG_DEBUG, "Allocating address space");
  MakeUserSpace(); // ###

  /*
   * Make sure the static image pages are marked writable before we try
   * to write them.
   */
  ZLOGS(LOG_DEBUG, "Loading into memory");
  err = Zmprotect((void*)MEM_START + NACL_TRAMPOLINE_START,
      ROUNDUP_64K(nap->data_end) - NACL_TRAMPOLINE_START,
      PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != err, err, "Failed to make image pages writable");

  ElfImageLoad(image, gp, MEM_START);

  /*
   * FillEndOfTextRegion will fill with halt instructions the
   * padding space after the static text region.
   *
   * Shm-backed dynamic text space was filled with halt instructions
   * in NaClMakeDynamicTextShared.  This extends to the rodata.  For
   * non-shm-backed text space, this extend to the next page (and not
   * allocation page).  static_text_end is updated to include the
   * padding.
   */
  FillEndOfTextRegion(nap);

  ZLOGS(LOG_DEBUG, "Initializing arch switcher");
  InitSwitchToApp(nap);

  ZLOGS(LOG_DEBUG, "Applying memory protection");
  SetUserSpace(); // ###

  ZLOGS(LOG_DEBUG, "AppLoadFile done");
  LogAddressSpaceLayout(nap);

  ElfImageDelete(image);
}
#undef DUMP

NORETURN void RunSession(struct NaClApp *nap)
{
  uintptr_t stack_ptr;

  assert(nap != NULL);

  /* set up user stack */
  stack_ptr = MEM_START + ((uintptr_t)1U << ADDR_BITS);
  stack_ptr -= STACK_USER_DATA_SIZE;
  memset((void*)stack_ptr, 0, STACK_USER_DATA_SIZE);
  ((uint32_t*)stack_ptr)[4] = 1;
  ((uint32_t*)stack_ptr)[5] = 0xfffffff0;

  /*
   * construct "nacl_user" and "nacl_sys" globals
   * note: nacl_sys->prog_ctr meaningless but should not be 0
   */
  ThreadContextCtor(nacl_user, nap->initial_entry_pt, stack_ptr);
  ThreadContextCtor(nacl_sys, 1, GetStackPtr());

  /* pass control to the user side */
  ZLOGS(LOG_DEBUG, "SESSION %s STARTED", nap->manifest->node);
  ContextSwitch(nacl_user);
  ZLOGFAIL(1, EFAULT, "the unreachable has been reached");
}
