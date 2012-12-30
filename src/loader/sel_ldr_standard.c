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
#include "src/syscalls/nacl_switch_to_app.h"
#include "src/platform/sel_memory.h"
#include "src/loader/sel_addrspace.h"
#include "src/main/manifest_setup.h"
#include "src/main/nacl_globals.h"
#include "src/platform/nacl_signal.h"
#include "src/syscalls/nacl_switch_to_app.h"
#include "src/main/zlog.h"
#include "src/main/nacl_config.h"

/*
 * d'b: alternative mechanism to pass control to user side
 * note: initializes "nacl_user" global
 */
NORETURN void SwitchToApp(struct NaClApp  *nap, uintptr_t stack_ptr)
{
  /* initialize "nacl_user" global */
  if(!nacl_user) nacl_user = g_malloc(sizeof(*nacl_user));

  /* construct "nacl_user" global */
  NaClThreadContextCtor(nacl_user, nap, nap->initial_entry_pt,
      NaClSysToUserStackAddr(nap, stack_ptr), 0);
  nacl_user->sysret = nap->break_addr;
  nacl_user->prog_ctr = NaClUserToSys(nap, nap->initial_entry_pt);
  nacl_user->new_prog_ctr = NaClUserToSys(nap, nap->initial_entry_pt);

  /* initialize "nacl_sys" global */
  if(!nacl_sys) nacl_sys = g_malloc(sizeof(*nacl_sys));

  nacl_sys->rbp = NaClGetStackPtr();
  nacl_sys->rsp = NaClGetStackPtr();

  /* pass control to the nexe */
  NaClSwitchToApp(nap, nacl_user->new_prog_ctr);

  ZLOGFAIL(1, EFAULT, "unreachable code reached");
}
/* d'b end */

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
void NaClFillEndOfTextRegion(struct NaClApp *nap) {
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

  if(NULL == nap->text_shm)
  {
    /* No dynamic text exists.  Space for NACL_HALT_SLED_SIZE must exist */
    page_pad = NaClRoundAllocPage(nap->static_text_end + NACL_HALT_SLED_SIZE)
        - nap->static_text_end;
    ZLOGFAIL(page_pad < NACL_HALT_SLED_SIZE, EFAULT, FAILED_MSG);
    ZLOGFAIL(page_pad >= NACL_MAP_PAGESIZE + NACL_HALT_SLED_SIZE, EFAULT, FAILED_MSG);
  }
  else
  {
    /*
     * Dynamic text exists; the halt sled resides in the dynamic text
     * region, so all we need to do here is to round out the last
     * static text page with HLT instructions.  It doesn't matter if
     * the size of this region is smaller than NACL_HALT_SLED_SIZE --
     * this is just to fully initialize the page, rather than (later)
     * decoding/validating zero-filled memory as instructions.
     */
    page_pad = NaClRoundAllocPage(nap->static_text_end) - nap->static_text_end;
  }

  ZLOGS(LOG_DEBUG, "Filling with halts: %08lx, %08lx bytes",
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
    ZLOGFAIL(NaClRoundAllocPage(rodata_end) != max_vaddr, ENOEXEC, FAILED_MSG);

  /* fail if Read-only data segment overlaps data segment */
  if(0 != nap->rodata_start && 0 != nap->data_start)
    ZLOGFAIL(rodata_end > nap->data_start, ENOEXEC, FAILED_MSG);

  /* fail if Text segment overlaps rodata segment */
  if(0 != nap->rodata_start)
    ZLOGFAIL(NaClRoundAllocPage(NaClEndOfStaticText(nap)) > nap->rodata_start,
        ENOEXEC, FAILED_MSG);
  /* fail if No rodata segment, and text segment overlaps data segment */
  else if(0 != nap->data_start)
    ZLOGFAIL(NaClRoundAllocPage(NaClEndOfStaticText(nap)) > nap->data_start,
        ENOEXEC, FAILED_MSG);

  /* fail if rodata_start not a multiple of allocation size */
  ZLOGFAIL(0 != nap->rodata_start && NaClRoundAllocPage(nap->rodata_start)
    != nap->rodata_start, ENOEXEC, FAILED_MSG);

  /* fail if data_start not a multiple of allocation size */
  ZLOGFAIL(0 != nap->data_start && NaClRoundAllocPage(nap->data_start)
    != nap->data_start, ENOEXEC, FAILED_MSG);
}

#define DUMP(a) ZLOGS(LOG_DEBUG, "%-24s = 0x%016x", #a, a)
void NaClLogAddressSpaceLayout(struct NaClApp *nap)
{
  ZLOGS(LOG_DEBUG, "NaClApp addr space layout:");
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

void NaClAppLoadFile(struct Gio *gp, struct NaClApp *nap)
{
  uintptr_t rodata_end;
  uintptr_t data_end;
  uintptr_t max_vaddr;
  struct NaClElfImage *image = NULL;
  int err;

  /* fail if Address space too big */
  ZLOGFAIL(nap->addr_bits > NACL_MAX_ADDR_BITS, EFAULT, FAILED_MSG);

  nap->stack_size = NaClRoundAllocPage(nap->stack_size);

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
      if(NaClRoundAllocPage(max_vaddr) - max_vaddr < NACL_HALT_SLED_SIZE)
      {
        max_vaddr += NACL_MAP_PAGESIZE;
      }
    }
    max_vaddr = NaClRoundAllocPage(max_vaddr);
  }

  /*
   * max_vaddr -- the break or the boundary between data (initialized
   * and bss) and the address space hole -- does not have to be at a
   * page boundary.
   */
  nap->break_addr = max_vaddr;
  nap->data_end = max_vaddr;

  ZLOGS(LOG_DEBUG, "Values from NaClElfImageValidateProgramHeaders:");
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
      NaClRoundAllocPage(nap->data_end) - NACL_TRAMPOLINE_START,
      PROT_READ | PROT_WRITE);
  ZLOGFAIL(0 != err, EFAULT, "Failed to make image pages writable. code 0x%x", err);

  NaClElfImageLoad(image, gp, nap->addr_bits, nap->mem_start);

  /* d'b: shared memory for the dynamic text disabled */
  nap->dynamic_text_start = NaClRoundAllocPage(NaClEndOfStaticText(nap));
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

  ZLOGS(LOG_DEBUG, "NaClAppLoadFile done; ");
  NaClLogAddressSpaceLayout(nap);

  NaClElfImageDelete(image);
}
#undef DUMP

int NaClAddrIsValidEntryPt(struct NaClApp *nap, uintptr_t addr)
{
  if(0 != (addr & (nap->bundle_size - 1))) return 0;
  return addr < nap->static_text_end;
}

/*
 * preconditions:
 * argc > 0, argc and argv table is consistent
 * envv may be NULL (this happens on MacOS/Cocoa
 * if envv is non-NULL it is 'consistent', null terminated etc.
 */
int NaClCreateMainThread(struct NaClApp *nap)
{
  /* Compute size of string tables for argv and envv */
  int                   retval;
  int                   envc;
  size_t                size;
  int                   auxv_entries;
  size_t                ptr_tbl_size;
  int                   i;
  uint32_t              *p;
  char                  *strp;
  size_t                *argv_len;
  size_t                *envv_len;
  uintptr_t             stack_ptr;

  /* d'b {{ */
  int                   argc;
  char                  **argv;
  char const *const     *envv;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  argc = nap->system_manifest->cmd_line_size;
  argv = nap->system_manifest->cmd_line;
  envv = (char const* const*)nap->system_manifest->envp;
  /* }} */

  retval = 0;  /* fail */
  ZLOGFAIL(argc <= 0, EFAULT, FAILED_MSG);
  ZLOGFAIL(NULL == argv, EFAULT, FAILED_MSG);

  envc = 0;
  if(NULL != envv)
  {
    char const * const *pp;
    for(pp = envv; NULL != *pp; ++pp)
      ++envc;
  }

  envv_len = 0;
  argv_len = g_malloc(argc * sizeof argv_len[0]);
  envv_len = g_malloc(envc * sizeof envv_len[0]);

  size = 0;

  /*
   * The following two loops cannot overflow.  The reason for this is
   * that they are counting the number of bytes used to hold the
   * NUL-terminated strings that comprise the argv and envv tables.
   * If the entire address space consisted of just those strings, then
   * the size variable would overflow; however, since there's the code
   * space required to hold the code below (and we are not targetting
   * Harvard architecture machines), at least one page holds code, not
   * data.  We are assuming that the caller is non-adversarial and the
   * code does not look like string data....
   */

  for(i = 0; i < argc; ++i)
  {
    argv_len[i] = strlen(argv[i]) + 1;
    size += argv_len[i];
  }

  for(i = 0; i < envc; ++i)
  {
    envv_len[i] = strlen(envv[i]) + 1;
    size += envv_len[i];
  }

  /*
   * NaCl modules are ILP32, so the argv, envv pointers, as well as
   * the terminating NULL pointers at the end of the argv/envv tables,
   * are 32-bit values.  We also have the auxv to take into account.
   *
   * The argv and envv pointer tables came from trusted code and is
   * part of memory.  Thus, by the same argument above, adding in
   * "ptr_tbl_size" cannot possibly overflow the "size" variable since
   * it is a size_t object.  However, the extra pointers for auxv and
   * the space for argv could cause an overflow.  The fact that we
   * used stack to get here etc means that ptr_tbl_size could not have
   * overflowed.
   *
   * NB: the underlying OS would have limited the amount of space used
   * for argv and envv -- on linux, it is ARG_MAX, or 128KB -- and
   * hence the overflow check is for obvious auditability rather than
   * for correctness.
   */
  auxv_entries = 1;
  if(0 != nap->user_entry_pt)
    auxv_entries++;

  ptr_tbl_size = (sizeof(uint32_t) *
      ((NACL_STACK_GETS_ARG ? 1 : 0) + (3 + argc + 1 + envc + 1 + auxv_entries * 2)));

  if(SIZE_T_MAX - size < ptr_tbl_size)
  {
    ZLOG(LOG_ERROR, "NaClCreateMainThread: ptr_tbl_size cause size of"
        " argv / environment copy to overflow!?!");
    retval = 0;
    goto cleanup;
  }
  size += ptr_tbl_size;

  size = (size + NACL_STACK_ALIGN_MASK) & ~NACL_STACK_ALIGN_MASK;

  if(size > nap->stack_size)
  {
    retval = 0;
    goto cleanup;
  }

  /* write strings and char * arrays to stack */
  stack_ptr = (nap->mem_start + ((uintptr_t) 1U << nap->addr_bits) - size);

  ZLOGS(LOG_DEBUG, "setting stack to : %016lx", stack_ptr);

  ZLOGFAIL(0 != (stack_ptr & NACL_STACK_ALIGN_MASK), EFAULT,
      "stack_ptr not aligned: %016x", stack_ptr);

  p = (uint32_t *) stack_ptr;
  strp = (char *) stack_ptr + ptr_tbl_size;

  /*
   * For x86-32, we push an initial argument that is the address of
   * the main argument block.  For other machines, this is passed
   * in a register and that's set in NaClStartThreadInApp.
   */
  if (NACL_STACK_GETS_ARG) {
    uint32_t *argloc = p++;
    *argloc = (uint32_t) NaClSysToUser(nap, (uintptr_t) p);
  }

  *p++ = 0;  /* Cleanup function pointer, always NULL.  */
  *p++ = envc;
  *p++ = argc;

  for(i = 0; i < argc; ++i)
  {
    *p++ = (uint32_t)NaClSysToUser(nap, (uintptr_t)strp);
    ZLOGS(LOG_DEBUG, "copying arg %d %p -> %p", i, argv[i], strp);
    strcpy(strp, argv[i]);
    strp += argv_len[i];
  }
  *p++ = 0;  /* argv[argc] is NULL.  */

  for(i = 0; i < envc; ++i)
  {
    *p++ = (uint32_t)NaClSysToUser(nap, (uintptr_t)strp);
    ZLOGS(LOG_DEBUG, "copying env %d %p -> %p", i, envv[i], strp);
    strcpy(strp, envv[i]);
    strp += envv_len[i];
  }
  *p++ = 0; /* envp[envc] is NULL.  */

  /* Push an auxv */
  if(0 != nap->user_entry_pt)
  {
    *p++ = AT_ENTRY;
    *p++ = (uint32_t)nap->user_entry_pt;
  }
  *p++ = AT_NULL;
  *p++ = 0;

  ZLOGFAIL((char*)p != (char*)stack_ptr + ptr_tbl_size, EFAULT, FAILED_MSG);

  /*
   * For x86, we adjust the stack pointer down to push a dummy return
   * address.  This happens after the stack pointer alignment.
   */
  stack_ptr -= NACL_STACK_PAD_BELOW_ALIGN;
  memset((void *) stack_ptr, 0, NACL_STACK_PAD_BELOW_ALIGN);

  ZLOGS(LOG_DEBUG, "system stack ptr: %016lx", stack_ptr);
  ZLOGS(LOG_DEBUG, "user stack ptr: %016lx", NaClSysToUserStackAddr(nap, stack_ptr));

  /* d'b: jump directly to user code instead of using thread launching */
  SwitchToApp(nap, stack_ptr);

  retval = 1;
cleanup:
  g_free(argv_len);
  g_free(envv_len);

  return retval;
}
