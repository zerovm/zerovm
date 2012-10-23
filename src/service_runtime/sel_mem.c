/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL) memory map.
 * todo(d'b): replace this functionality or even remove
 */
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include "src/service_runtime/zlog.h"
#include "src/service_runtime/sel_mem.h"
#include "src/service_runtime/sel_util.h"

#define START_ENTRIES   5   /* tramp+text, rodata, data, bss, stack */
#define REMOVE_MARKED_DEBUG 0

/*
 * The memory map structure is a simple array of memory regions which
 * may have different access protections.  We do not yet merge regions
 * with the same access protections together to reduce the region
 * number, but may do so in the future.
 *
 * Regions are described by (relative) starting page number, the
 * number of pages, and the protection that the pages should have.
 *
 * Takes ownership of NaClMemObj.
 */
struct NaClVmmapEntry *NaClVmmapEntryMake(uintptr_t             page_num,
                                          size_t                npages,
                                          int                   prot,
                                          struct NaClMemObj     *nmop) {
  struct NaClVmmapEntry *entry;

  ZLOG(LOG_INSANE, "NaClVmmapEntryMake(0x%lx, 0x%lx, 0x%x, 0x%lx)",
      page_num, npages, prot, (uintptr_t)nmop);
  entry = (struct NaClVmmapEntry *) malloc(sizeof *entry);
  if (NULL == entry) return 0;

  ZLOG(LOG_INSANE, "entry: 0x%lx", (uintptr_t) entry);
  entry->page_num = page_num;
  entry->npages = npages;
  entry->prot = prot;
  entry->nmop = nmop;
  entry->removed = 0;
  return entry;
}

void NaClVmmapEntryFree(struct NaClVmmapEntry *entry)
{
  ZLOG(LOG_INSANE, "NaClVmmapEntryFree(0x%08lx): (0x%lx, 0x%lx, 0x%x, 0x%lx)",
      (uintptr_t)entry, entry->page_num, entry->npages, entry->prot, (uintptr_t)entry->nmop);

  NaClMemObjSafeDtor(entry->nmop);
  free(entry->nmop);

  free(entry);
}

int NaClVmmapCtor(struct NaClVmmap *self)
{
  self->size = START_ENTRIES;
  if(SIZE_T_MAX / sizeof *self->vmentry < self->size)
  {
    return 0;
  }

  self->vmentry = calloc(self->size, sizeof *self->vmentry);
  if(!self->vmentry)
  {
    return 0;
  }

  self->nvalid = 0;
  self->is_sorted = 1;
  return 1;
}

/*
 * d'b: useless. kept because of unit test. should be removed
 * together with the rest of sel_mem functions
 */
void NaClVmmapDtor(struct NaClVmmap *self) {
  size_t i;

  for (i = 0; i < self->nvalid; ++i) {
    NaClVmmapEntryFree(self->vmentry[i]);
  }
  free(self->vmentry);
  self->vmentry = 0;
}

/*
 * Comparison function for qsort.  Should never encounter a
 * removed/invalid entry.
 */

static int NaClVmmapCmpEntries(void const  *vleft,
                               void const  *vright) {
  struct NaClVmmapEntry const *const *left =
      (struct NaClVmmapEntry const *const *) vleft;
  struct NaClVmmapEntry const *const *right =
      (struct NaClVmmapEntry const *const *) vright;

  return (int) ((*left)->page_num - (*right)->page_num);
}

static void NaClVmmapRemoveMarked(struct NaClVmmap *self) {
  size_t  i;
  size_t  last;

  if (0 == self->nvalid)
    return;

#if REMOVE_MARKED_DEBUG
  NaClVmmapDebug(self, "Before RemoveMarked");
#endif
  /*
   * Linearly scan with a move-end-to-front strategy to get rid of
   * marked-to-be-removed entries.
   */

  /*
   * Invariant:
   *
   * forall j in [0, self->nvalid): NULL != self->vmentry[j]
   */
  for (last = self->nvalid; last > 0 && self->vmentry[--last]->removed; ) {
    NaClVmmapEntryFree(self->vmentry[last]);
    self->vmentry[last] = NULL;
  }
  ZLOGFAIL(last == 0 && self->vmentry[0]->removed, EFAULT, "No valid entries in VM map");

  /*
   * Post condition of above loop:
   *
   * forall j in [0, last]: NULL != self->vmentry[j]
   *
   * 0 <= last < self->nvalid && !self->vmentry[last]->removed
   */
  ZLOGFAIL(last >= self->nvalid, EFAULT, FAILED_MSG);
  ZLOGFAIL(self->vmentry[last]->removed, EFAULT, FAILED_MSG);
  /*
   * and,
   *
   * forall j in (last, self->nvalid): NULL == self->vmentry[j]
   */

  /*
   * Loop invariant: forall j in [0, i):  !self->vmentry[j]->removed
   */
  for (i = 0; i < last; ++i) {
    if (!self->vmentry[i]->removed) {
      continue;
    }
    /*
     * post condition: self->vmentry[i]->removed
     *
     * swap with entry at self->vmentry[last].
     */

    NaClVmmapEntryFree(self->vmentry[i]);
    self->vmentry[i] = self->vmentry[last];
    self->vmentry[last] = NULL;

    /*
     * Invariants here:
     *
     * forall j in [last, self->nvalid): NULL == self->vmentry[j]
     *
     * forall j in [0, i]: !self->vmentry[j]->removed
     */

    while (--last > i && self->vmentry[last]->removed) {
      NaClVmmapEntryFree(self->vmentry[last]);
      self->vmentry[last] = NULL;
    }
    /*
     * since !self->vmentry[i]->removed, we are guaranteed that
     * !self->vmentry[last]->removed when the while loop terminates.
     *
     * forall j in (last, self->nvalid):
     *  NULL == self->vmentry[j]->removed
     */
  }
  /* i == last */
  /* forall j in [0, last]: !self->vmentry[j]->removed */
  /* forall j in (last, self->nvalid): NULL == self->vmentry[j] */
  self->nvalid = last + 1;

  self->is_sorted = 0;
#if REMOVE_MARKED_DEBUG
  NaClVmmapDebug(self, "After RemoveMarked");
#endif
}

void NaClVmmapMakeSorted(struct NaClVmmap  *self) {
  if (self->is_sorted)
    return;

  NaClVmmapRemoveMarked(self);

  qsort(self->vmentry,
        self->nvalid,
        sizeof *self->vmentry,
        NaClVmmapCmpEntries);
  self->is_sorted = 1;
#if REMOVE_MARKED_DEBUG
  NaClVmmapDebug(self, "After Sort");
#endif
}

/*
 * Adds an entry.  Does not sort.
 */
int NaClVmmapAdd(struct NaClVmmap   *self,
                 uintptr_t          page_num,
                 size_t             npages,
                 int                prot,
                 struct NaClMemObj  *nmop) {
  struct NaClVmmapEntry *entry;

  ZLOGS(LOG_DEBUG, "NaClVmmapAdd(0x%08lx, 0x%lx, 0x%lx, 0x%x, 0x%08lx)",
      (uintptr_t)self, page_num, npages, prot, (uintptr_t)nmop);

  if (self->nvalid == self->size) {
    size_t                    new_size = 2 * self->size;
    struct NaClVmmapEntry     **new_map;

    new_map = realloc(self->vmentry, new_size * sizeof *new_map);
    if (NULL == new_map) {
      return 0;
    }
    self->vmentry = new_map;
    self->size = new_size;
  }
  /* self->nvalid < self->size */
  entry = NaClVmmapEntryMake(page_num, npages, prot, nmop);

  self->vmentry[self->nvalid] = entry;
  self->is_sorted = 0;
  ++self->nvalid;

  return 1;
}

void  NaClVmmapVisit(struct NaClVmmap *self,
                     void             (*fn)(void                  *state,
                                            struct NaClVmmapEntry *entry),
                     void             *state) {
  size_t i;
  size_t nentries;

  NaClVmmapMakeSorted(self);
  for (i = 0, nentries = self->nvalid; i < nentries; ++i) {
    (*fn)(state, self->vmentry[i]);
  }
}
