/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */
#include <string.h>
#include <assert.h>

#include "src/platform/nacl_check.h"
#include "src/platform/nacl_sync_checked.h"
#include "src/perf_counter/nacl_perf_counter.h"
#include "src/service_runtime/include/bits/mman.h"
#include "src/service_runtime/arch/x86/sel_ldr_x86.h"
#include "src/service_runtime/elf_util.h"
#include "src/service_runtime/nacl_switch_to_app.h"
#include "src/service_runtime/nacl_text.h"
#include "src/service_runtime/sel_memory.h"
#include "src/service_runtime/sel_addrspace.h"
#include "src/manifest/manifest_setup.h" /* d'b: system_manifest */
#include "src/service_runtime/nacl_globals.h" /* d'b: nacl_user */
#include "src/service_runtime/nacl_signal.h"

#define NORETURN_PTR NORETURN
extern NORETURN void NaClSwitchSSE(struct NaClThreadContext *context);

/*
 * d'b: alternative mechanism to pass control to user side
 * note: initializes "nacl_user" global
 */
NORETURN void SwitchToApp(struct NaClApp  *nap, uintptr_t stack_ptr)
{
  /* initialize "nacl_user" global */
  if(!nacl_user) nacl_user = malloc(sizeof(*nacl_user));
  assert(nacl_user != NULL);

  /* construct "nacl_user" global */
  NaClThreadContextCtor(nacl_user, nap, nap->initial_entry_pt,
                        NaClSysToUserStackAddr(nap, stack_ptr), 0);
  assert(NaClSignalStackAllocate(&nap->signal_stack));
  nacl_user->sysret = nap->break_addr;
  nacl_user->prog_ctr = NaClUserToSys(nap, nap->initial_entry_pt);
  nacl_user->new_prog_ctr = NaClUserToSys(nap, nap->initial_entry_pt);

  /* initialize "nacl_sys" global */
  if(!nacl_sys) nacl_sys = malloc(sizeof(*nacl_sys));
  assert(nacl_sys != NULL);
  nacl_sys->rbp = NaClGetStackPtr();
  nacl_sys->rsp = NaClGetStackPtr();

  /* set state to "ok" just before the nexe start */
  sprintf(nap->zvm_state, "ok");

  /* todo: put here switch to chose proper function: avx or sse */
  nap->user_side_flag = 1;
  nap->trusted_code = 0;
  NaClSwitchSSE(nacl_user);

  NaClAbort(); /* unreachable */
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
  if (0 != nap->data_start &&
      nap->static_text_end + NACL_HALT_SLED_SIZE > nap->data_start) {
    NaClLog(LOG_FATAL, "Missing gap between text and data for halt_sled\n");
  }
  if (0 != nap->rodata_start &&
      nap->static_text_end + NACL_HALT_SLED_SIZE > nap->rodata_start) {
    NaClLog(LOG_FATAL, "Missing gap between text and rodata for halt_sled\n");
  }

  if (NULL == nap->text_shm) {
    /*
     * No dynamic text exists.  Space for NACL_HALT_SLED_SIZE must
     * exist.
     */
    page_pad = (NaClRoundAllocPage(nap->static_text_end + NACL_HALT_SLED_SIZE)
                - nap->static_text_end);
    CHECK(page_pad >= NACL_HALT_SLED_SIZE);
    CHECK(page_pad < NACL_MAP_PAGESIZE + NACL_HALT_SLED_SIZE);
  } else {
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

  NaClLog(4,
          "Filling with halts: %08"NACL_PRIxPTR", %08"NACL_PRIxS" bytes\n",
          nap->mem_start + nap->static_text_end,
          page_pad);

  NaClFillMemoryRegionWithHalt((void *)(nap->mem_start + nap->static_text_end),
                               page_pad);

  nap->static_text_end += page_pad;
}

/*
 * Basic address space layout sanity check.
 */
NaClErrorCode NaClCheckAddressSpaceLayoutSanity(struct NaClApp *nap,
                                                uintptr_t rodata_end,
                                                uintptr_t data_end,
                                                uintptr_t max_vaddr) {
  if (0 != nap->data_start) {
    if (data_end != max_vaddr) {
      NaClLog(LOG_INFO, "data segment is not last\n");
      return LOAD_DATA_NOT_LAST_SEGMENT;
    }
  } else if (0 != nap->rodata_start) {
    if (NaClRoundAllocPage(rodata_end) != max_vaddr) {
      /*
       * This should be unreachable, but we include it just for
       * completeness.
       *
       * Here is why it is unreachable:
       *
       * NaClPhdrChecks checks the test segment starting address.  The
       * only allowed loaded segments are text, data, and rodata.
       * Thus unless the rodata is in the trampoline region, it must
       * be after the text.  And NaClElfImageValidateProgramHeaders
       * ensures that all segments start after the trampoline region.
       */
      NaClLog(LOG_INFO, "no data segment, but rodata segment is not last\n");
      return LOAD_NO_DATA_BUT_RODATA_NOT_LAST_SEGMENT;
    }
  }
  if (0 != nap->rodata_start && 0 != nap->data_start) {
    if (rodata_end > nap->data_start) {
      NaClLog(LOG_INFO, "rodata_overlaps data.\n");
      return LOAD_RODATA_OVERLAPS_DATA;
    }
  }
  if (0 != nap->rodata_start) {
    if (NaClRoundAllocPage(NaClEndOfStaticText(nap)) > nap->rodata_start) {
      return LOAD_TEXT_OVERLAPS_RODATA;
    }
  } else if (0 != nap->data_start) {
    if (NaClRoundAllocPage(NaClEndOfStaticText(nap)) > nap->data_start) {
      return LOAD_TEXT_OVERLAPS_DATA;
    }
  }
  if (0 != nap->rodata_start &&
      NaClRoundAllocPage(nap->rodata_start) != nap->rodata_start) {
    NaClLog(LOG_INFO, "rodata_start not a multiple of allocation size\n");
    return LOAD_BAD_RODATA_ALIGNMENT;
  }
  if (0 != nap->data_start &&
      NaClRoundAllocPage(nap->data_start) != nap->data_start) {
    NaClLog(LOG_INFO, "data_start not a multiple of allocation size\n");
    return LOAD_BAD_DATA_ALIGNMENT;
  }
  return LOAD_OK;
}

void NaClLogAddressSpaceLayout(struct NaClApp *nap) {
  NaClLog(2, "NaClApp addr space layout:\n");
  NaClLog(2, "nap->static_text_end    = 0x%016"NACL_PRIxPTR"\n",
          nap->static_text_end);
  NaClLog(2, "nap->dynamic_text_start = 0x%016"NACL_PRIxPTR"\n",
          nap->dynamic_text_start);
  NaClLog(2, "nap->dynamic_text_end   = 0x%016"NACL_PRIxPTR"\n",
          nap->dynamic_text_end);
  NaClLog(2, "nap->rodata_start       = 0x%016"NACL_PRIxPTR"\n",
          nap->rodata_start);
  NaClLog(2, "nap->data_start         = 0x%016"NACL_PRIxPTR"\n",
          nap->data_start);
  NaClLog(2, "nap->data_end           = 0x%016"NACL_PRIxPTR"\n",
          nap->data_end);
  NaClLog(2, "nap->break_addr         = 0x%016"NACL_PRIxPTR"\n",
          nap->break_addr);
  NaClLog(2, "nap->initial_entry_pt   = 0x%016"NACL_PRIxPTR"\n",
          nap->initial_entry_pt);
  NaClLog(2, "nap->user_entry_pt      = 0x%016"NACL_PRIxPTR"\n",
          nap->user_entry_pt);
  NaClLog(2, "nap->bundle_size        = 0x%x\n", nap->bundle_size);
}

NaClErrorCode NaClAppLoadFile(struct Gio       *gp,
                              struct NaClApp   *nap) {
  NaClErrorCode       ret = LOAD_INTERNAL;
  NaClErrorCode       subret;
  uintptr_t           rodata_end;
  uintptr_t           data_end;
  uintptr_t           max_vaddr;
  struct NaClElfImage *image = NULL;
  struct NaClPerfCounter  time_load_file;

  NaClPerfCounterCtor(&time_load_file, "NaClAppLoadFile");

  /* NACL_MAX_ADDR_BITS < 32 */
  if (nap->addr_bits > NACL_MAX_ADDR_BITS) {
    ret = LOAD_ADDR_SPACE_TOO_BIG;
    goto done;
  }

  nap->stack_size = NaClRoundAllocPage(nap->stack_size);

  /* temporay object will be deleted at end of function */
  image = NaClElfImageNew(gp, &subret);
  if (NULL == image) {
    ret = subret;
    goto done;
  }

  subret = NaClElfImageValidateElfHeader(image);
  if (LOAD_OK != subret) {
    ret = subret;
    goto done;
  }

  subret = NaClElfImageValidateProgramHeaders(image,
                                              nap->addr_bits,
                                              &nap->static_text_end,
                                              &nap->rodata_start,
                                              &rodata_end,
                                              &nap->data_start,
                                              &data_end,
                                              &max_vaddr);
  if (LOAD_OK != subret) {
    ret = subret;
    goto done;
  }

  if (0 == nap->data_start) {
    if (0 == nap->rodata_start) {
      if (NaClRoundAllocPage(max_vaddr) - max_vaddr < NACL_HALT_SLED_SIZE) {
        /*
         * if no rodata and no data, we make sure that there is space for
         * the halt sled.
         */
        max_vaddr += NACL_MAP_PAGESIZE;
      }
    } else {
      /*
       * no data, but there is rodata.  this means max_vaddr is just
       * where rodata ends.  this might not be at an allocation
       * boundary, and in this the page would not be writable.  round
       * max_vaddr up to the next allocation boundary so that bss will
       * be at the next writable region.
       */
      ;
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

  NaClLog(4, "Values from NaClElfImageValidateProgramHeaders:\n");
  NaClLog(4, "rodata_start = 0x%08"NACL_PRIxPTR"\n", nap->rodata_start);
  NaClLog(4, "rodata_end   = 0x%08"NACL_PRIxPTR"\n", rodata_end);
  NaClLog(4, "data_start   = 0x%08"NACL_PRIxPTR"\n", nap->data_start);
  NaClLog(4, "data_end     = 0x%08"NACL_PRIxPTR"\n", data_end);
  NaClLog(4, "max_vaddr    = 0x%08"NACL_PRIxPTR"\n", max_vaddr);

  /* We now support only one bundle size.  */
  nap->bundle_size = NACL_INSTR_BLOCK_SIZE;

  nap->initial_entry_pt = NaClElfImageGetEntryPoint(image);
  NaClLogAddressSpaceLayout(nap);

  if (!NaClAddrIsValidEntryPt(nap, nap->initial_entry_pt)) {
    ret = LOAD_BAD_ENTRY;
    goto done;
  }

  subret = NaClCheckAddressSpaceLayoutSanity(nap, rodata_end, data_end,
                                             max_vaddr);
  if (LOAD_OK != subret) {
    ret = subret;
    goto done;
  }

  NaClLog(2, "Allocating address space\n");
  NaClPerfCounterMark(&time_load_file, "PreAllocAddrSpace");
  NaClPerfCounterIntervalLast(&time_load_file);
  subret = NaClAllocAddrSpace(nap);
  NaClPerfCounterMark(&time_load_file,
                      NACL_PERF_IMPORTANT_PREFIX "AllocAddrSpace");
  NaClPerfCounterIntervalLast(&time_load_file);
  if (LOAD_OK != subret) {
    ret = subret;
    goto done;
  }

  /*
   * Make sure the static image pages are marked writable before we try
   * to write them.
   */
  NaClLog(2, "Loading into memory\n");
  ret = NaCl_mprotect((void *) (nap->mem_start + NACL_TRAMPOLINE_START),
                      NaClRoundAllocPage(nap->data_end) - NACL_TRAMPOLINE_START,
                      NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE);
  if (0 != ret) {
    NaClLog(LOG_FATAL,
            "NaClAppLoadFile: Failed to make image pages writable. "
            "Error code 0x%x\n",
            ret);
  }
  subret = NaClElfImageLoad(image, gp, nap->addr_bits, nap->mem_start);
  if (LOAD_OK != subret) {
    ret = subret;
    goto done;
  }

  /*
   * NB: mem_map object has been initialized, but is empty.
   * NaClMakeDynamicTextShared does not touch it.
   *
   * NaClMakeDynamicTextShared also fills the dynamic memory region
   * with the architecture-specific halt instruction.  If/when we use
   * memory mapping to save paging space for the dynamic region and
   * lazily halt fill the memory as the pages become
   * readable/executable, we must make sure that the *last*
   * NACL_MAP_PAGESIZE chunk is nonetheless mapped and written with
   * halts.
   */
  NaClLog(2,
          ("Replacing gap between static text and"
           " (ro)data with shareable memory\n"));
  subret = NaClMakeDynamicTextShared(nap);
  NaClPerfCounterMark(&time_load_file,
                      NACL_PERF_IMPORTANT_PREFIX "MakeDynText");
  NaClPerfCounterIntervalLast(&time_load_file);
  if (LOAD_OK != subret) {
    ret = subret;
    goto done;
  }

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

  NaClLog(2, "Initializing arch switcher\n");
  NaClInitSwitchToApp(nap);

  NaClLog(2, "Installing trampoline\n");
  NaClLoadTrampoline(nap);

  /*
   * NaClMemoryProtect also initializes the mem_map w/ information
   * about the memory pages and their current protection value.
   *
   * The contents of the dynamic text region will get remapped as
   * non-writable.
   */
  NaClLog(2, "Applying memory protection\n");
  subret = NaClMemoryProtection(nap);
  if (LOAD_OK != subret) {
    ret = subret;
    goto done;
  }

  NaClLog(2, "NaClAppLoadFile done; ");
  NaClLogAddressSpaceLayout(nap);
  ret = LOAD_OK;
done:
  NaClElfImageDelete(image);

  NaClPerfCounterMark(&time_load_file, "EndLoadFile");
  NaClPerfCounterIntervalTotal(&time_load_file);
  return ret;
}

int NaClAddrIsValidEntryPt(struct NaClApp *nap,
                           uintptr_t      addr) {
  if (0 != (addr & (nap->bundle_size - 1))) {
    return 0;
  }

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
  /*
   * Compute size of string tables for argv and envv
   */
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
  CHECK(argc > 0);
  CHECK(NULL != argv);

  envc = 0;
  if (NULL != envv) {
    char const *const *pp;
    for (pp = envv; NULL != *pp; ++pp) {
      ++envc;
    }
  }
  envv_len = 0;
  argv_len = malloc(argc * sizeof argv_len[0]);
  envv_len = malloc(envc * sizeof envv_len[0]);
  if (NULL == argv_len) {
    goto cleanup;
  }
  if (NULL == envv_len && 0 != envc) {
    goto cleanup;
  }

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

  for (i = 0; i < argc; ++i) {
    argv_len[i] = strlen(argv[i]) + 1;
    size += argv_len[i];
  }
  for (i = 0; i < envc; ++i) {
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
  if (0 != nap->user_entry_pt) {
    auxv_entries++;
  }
  ptr_tbl_size = (((NACL_STACK_GETS_ARG ? 1 : 0) +
                   (3 + argc + 1 + envc + 1 + auxv_entries * 2)) *
                  sizeof(uint32_t));

  if (SIZE_T_MAX - size < ptr_tbl_size) {
    NaClLog(LOG_WARNING,
            "NaClCreateMainThread: ptr_tbl_size cause size of"
            " argv / environment copy to overflow!?!\n");
    retval = 0;
    goto cleanup;
  }
  size += ptr_tbl_size;

  size = (size + NACL_STACK_ALIGN_MASK) & ~NACL_STACK_ALIGN_MASK;

  if (size > nap->stack_size) {
    retval = 0;
    goto cleanup;
  }

  /* write strings and char * arrays to stack */
  stack_ptr = (nap->mem_start + ((uintptr_t) 1U << nap->addr_bits) - size);

  NaClLog(2, "setting stack to : %016"NACL_PRIxPTR"\n", stack_ptr);

  VCHECK(0 == (stack_ptr & NACL_STACK_ALIGN_MASK),
         ("stack_ptr not aligned: %016"NACL_PRIxPTR"\n", stack_ptr));

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

  for (i = 0; i < argc; ++i) {
    *p++ = (uint32_t) NaClSysToUser(nap, (uintptr_t) strp);
    NaClLog(2, "copying arg %d  %p -> %p\n",
            i, argv[i], strp);
    strcpy(strp, argv[i]);
    strp += argv_len[i];
  }
  *p++ = 0;  /* argv[argc] is NULL.  */

  for (i = 0; i < envc; ++i) {
    *p++ = (uint32_t) NaClSysToUser(nap, (uintptr_t) strp);
    NaClLog(2, "copying env %d  %p -> %p\n",
            i, envv[i], strp);
    strcpy(strp, envv[i]);
    strp += envv_len[i];
  }
  *p++ = 0;  /* envp[envc] is NULL.  */

  /* Push an auxv */
  if (0 != nap->user_entry_pt) {
    *p++ = AT_ENTRY;
    *p++ = (uint32_t) nap->user_entry_pt;
  }
  *p++ = AT_NULL;
  *p++ = 0;

  CHECK((char *) p == (char *) stack_ptr + ptr_tbl_size);

  /*
   * For x86, we adjust the stack pointer down to push a dummy return
   * address.  This happens after the stack pointer alignment.
   */
  stack_ptr -= NACL_STACK_PAD_BELOW_ALIGN;
  memset((void *) stack_ptr, 0, NACL_STACK_PAD_BELOW_ALIGN);

  NaClLog(2, "system stack ptr : %016"NACL_PRIxPTR"\n", stack_ptr);
  NaClLog(2, "  user stack ptr : %016"NACL_PRIxPTR"\n",
          NaClSysToUserStackAddr(nap, stack_ptr));

  /* d'b: jump directly to user code instead of using thread launching */
  SwitchToApp(nap, stack_ptr);

  retval = 1;
cleanup:
  free(argv_len);
  free(envv_len);

  return retval;
}
