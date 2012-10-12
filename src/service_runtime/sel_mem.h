/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL) memory map.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_SEL_MEM_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_SEL_MEM_H_

#include "src/service_runtime/nacl_memory_object.h"

EXTERN_C_BEGIN

/*
 * Interface is based on setting properties and query properties by
 * page numbers (addr >> NACL_PAGESHIFT) and the number of pages
 * affected (for setting properties).
 *
 * Initially, the address space is empty, with all memory
 * inaccessible.  As the program is loaded, pages are marked
 * accessible -- text pages are non-writable, data and stack are
 * writable.  At runtime, shared memory buffers are allocated by
 * looking at the first memory hole that fits, starting down from the
 * stack.
 *
 * The simple data structure that we use is a sorted array of valid
 * memory regions.
 */

struct NaClVmmapEntry {
  uintptr_t             page_num;   /* base virtual address >> NACL_PAGESHIFT */
  size_t                npages;     /* number of pages */
  int                   prot;       /* mprotect attribute */
  struct NaClMemObj     *nmop;      /* how to get memory for move/remap */
  int                   removed;    /* flag set in NaClVmmapUpdate */
};

struct NaClVmmap {
  struct NaClVmmapEntry **vmentry;       /* must not overlap */
  size_t                nvalid, size;
  int                   is_sorted;
};

int   NaClVmmapCtor(struct NaClVmmap  *self) NACL_WUR;

void  NaClVmmapDtor(struct NaClVmmap  *self);

int   NaClVmmapAdd(struct NaClVmmap   *self,
                   uintptr_t          page_num,
                   size_t             npages,
                   int                prot,
                   struct NaClMemObj  *nmop) NACL_WUR;

/*
 * Visitor pattern, call fn on every entry.
 */
void  NaClVmmapVisit(struct NaClVmmap   *self,
                     void               (*fn)(void                  *state,
                                              struct NaClVmmapEntry *entry),
                     void               *state);

void NaClVmmapMakeSorted(struct NaClVmmap  *self);

EXTERN_C_END

#endif
