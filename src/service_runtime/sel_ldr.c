/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */
#include "src/service_runtime/zlog.h"
#include "src/gio/gio.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/sel_ldr_x86.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_syscall_handlers.h"

int NaClAppWithSyscallTableCtor(struct NaClApp               *nap,
                                struct NaClSyscallTableEntry *table)
{
  nap->addr_bits = NACL_MAX_ADDR_BITS;
  nap->stack_size = NACL_DEFAULT_STACK_MAX;
  nap->mem_start = 0;
  nap->dispatch_thunk = 0;
  nap->static_text_end = 0;
  nap->dynamic_text_start = 0;
  nap->dynamic_text_end = 0;
  nap->rodata_start = 0;
  nap->data_start = 0;
  nap->data_end = 0;

  nap->initial_entry_pt = 0;
  nap->user_entry_pt = 0;

  if(!NaClVmmapCtor(&nap->mem_map))
  {
    return 0;
  }

  nap->text_shm = NULL;
  nap->dynamic_page_bitmap = NULL;

  nap->dynamic_regions = NULL;
  nap->num_dynamic_regions = 0;
  nap->dynamic_regions_allocated = 0;

  nap->dynamic_mapcache_offset = 0;
  nap->dynamic_mapcache_size = 0;
  nap->dynamic_mapcache_ret = 0;

  nap->syscall_table = table;

  return 1;
}

int NaClAppCtor(struct NaClApp *nap) {
  return NaClAppWithSyscallTableCtor(nap, nacl_syscall);
}

/*
 * unaligned little-endian load.  precondition: nbytes should never be
 * more than 8.
 */
static uint64_t NaClLoadMem(uintptr_t addr, size_t user_nbytes)
{
  uint64_t value = 0;

  ZLOGFAIL(0 == user_nbytes || user_nbytes > 8, EFAULT, FAILED_MSG);

  do
  {
    value = value << 8;
    value |= ((uint8_t *)addr)[--user_nbytes];
  } while(user_nbytes > 0);

  return value;
}

#define GENERIC_LOAD(bits) \
  static uint ## bits ## _t NaClLoad ## bits(uintptr_t addr) { \
    return (uint ## bits ## _t) NaClLoadMem(addr, sizeof(uint ## bits ## _t)); \
  }

GENERIC_LOAD(64)

#undef GENERIC_LOAD

/* unaligned little-endian store */
static void NaClStoreMem(uintptr_t addr, size_t nbytes, uint64_t value)
{
  size_t i;

  ZLOGFAIL(nbytes > 8, EFAULT, FAILED_MSG);

  for(i = 0; i < nbytes; ++i)
  {
    ((uint8_t *)addr)[i] = (uint8_t)value;
    value = value >> 8;
  }
}

#define GENERIC_STORE(bits) \
  static void NaClStore ## bits(uintptr_t addr, \
                                uint ## bits ## _t v) { \
    NaClStoreMem(addr, sizeof(uint ## bits ## _t), v); \
  }

GENERIC_STORE(16)
GENERIC_STORE(32)
GENERIC_STORE(64)

#undef GENERIC_STORE

struct NaClPatchInfo *NaClPatchInfoCtor(struct NaClPatchInfo *self) {
  if (NULL != self) {
    memset(self, 0, sizeof *self);
  }
  return self;
}

/*
 * This function is called by NaClLoadTrampoline and NaClLoadSpringboard to
 * patch a single memory location specified in NaClPatchInfo structure.
 */
void  NaClApplyPatchToMemory(struct NaClPatchInfo  *patch) {
  size_t    i;
  size_t    offset;
  int64_t   reloc;
  uintptr_t target_addr;

  memcpy((void *) patch->dst, (void *) patch->src, patch->nbytes);

  reloc = patch->dst - patch->src;


  for (i = 0; i < patch->num_abs64; ++i) {
    offset = patch->abs64[i].target - patch->src;
    target_addr = patch->dst + offset;
    NaClStore64(target_addr, patch->abs64[i].value);
  }

  for (i = 0; i < patch->num_abs32; ++i) {
    offset = patch->abs32[i].target - patch->src;
    target_addr = patch->dst + offset;
    NaClStore32(target_addr, (uint32_t) patch->abs32[i].value);
  }

  for (i = 0; i < patch->num_abs16; ++i) {
    offset = patch->abs16[i].target - patch->src;
    target_addr = patch->dst + offset;
    NaClStore16(target_addr, (uint16_t) patch->abs16[i].value);
  }

  for (i = 0; i < patch->num_rel64; ++i) {
    offset = patch->rel64[i] - patch->src;
    target_addr = patch->dst + offset;
    NaClStore64(target_addr, NaClLoad64(target_addr) - reloc);
  }
}

/*
 * Install syscall trampolines at all possible well-formed entry
 * points within the trampoline pages.  Many of these syscalls will
 * correspond to unimplemented system calls and will just abort the
 * program.
 */
void  NaClLoadTrampoline(struct NaClApp *nap) {
  int         num_syscalls;
  int         i;
  uintptr_t   addr;

  ZLOGFAIL(!NaClMakeDispatchThunk(nap), EFAULT, FAILED_MSG);
  NaClFillTrampolineRegion(nap);

  /*
   * Do not bother to fill in the contents of page 0, since we make it
   * inaccessible later (see sel_addrspace.c, NaClMemoryProtection)
   * anyway to help detect NULL pointer errors, and we might as well
   * not dirty the page.
   *
   * The last syscall entry point is used for springboard code.
   */
  num_syscalls = ((NACL_TRAMPOLINE_END - NACL_SYSCALL_START_ADDR)
                  / NACL_SYSCALL_BLOCK_SIZE) - 1;

  ZLOGS(LOG_DEBUG, "num_syscalls = %d (0x%x)", num_syscalls, num_syscalls);

  for (i = 0, addr = nap->mem_start + NACL_SYSCALL_START_ADDR;
       i < num_syscalls;
       ++i, addr += NACL_SYSCALL_BLOCK_SIZE) {
    NaClPatchOneTrampoline(nap, addr);
  }
}

static int verb = LOG_INSANE;
void NaClMemRegionPrinter(void *state, struct NaClVmmapEntry *entry)
{
  ZLOGS(verb, "Page   %ld (0x%lx)", entry->page_num, entry->page_num);
  ZLOGS(verb, "npages %ld (0x%lx)", entry->npages, entry->npages);
  ZLOGS(verb, "start vaddr 0x%lx", entry->page_num << NACL_PAGESHIFT);
  ZLOGS(verb, "end vaddr   0x%lx", (entry->page_num + entry->npages) << NACL_PAGESHIFT);
  ZLOGS(verb, "prot   0x%08x", entry->prot);
  ZLOGS(verb, "%ssrc/backed by a file", (NULL == entry->nmop) ? "not " : "");
}

void NaClAppPrintDetails(struct NaClApp *nap, struct Gio *gp, int verbosity)
{
  verb = verbosity;
  ZLOGS(verbosity, "NaClAppPrintDetails((struct NaClApp *) 0x%08lx,"
      "(struct Gio *) 0x%08lx)", (uintptr_t)nap, (uintptr_t)gp);
  ZLOGS(verbosity, "addr space size:  2**%d", nap->addr_bits);
  ZLOGS(verbosity, "stack size:       0x%08d", nap->stack_size);
  ZLOGS(verbosity, "mem start addr:   0x%08lx", nap->mem_start);
  ZLOGS(verbosity, "static_text_end:   0x%08lx", nap->static_text_end);
  ZLOGS(verbosity, "end-of-text:       0x%08lx", NaClEndOfStaticText(nap));
  ZLOGS(verbosity, "rodata:            0x%08lx", nap->rodata_start);
  ZLOGS(verbosity, "data:              0x%08lx", nap->data_start);
  ZLOGS(verbosity, "data_end:          0x%08lx", nap->data_end);
  ZLOGS(verbosity, "break_addr:        0x%08lx", nap->break_addr);
  ZLOGS(verbosity, "ELF initial entry point:  0x%08x", nap->initial_entry_pt);
  ZLOGS(verbosity, "ELF user entry point:  0x%08x", nap->user_entry_pt);
  ZLOGS(verbosity, "memory map:");
  NaClVmmapVisit(&nap->mem_map, NaClMemRegionPrinter, gp);
}
