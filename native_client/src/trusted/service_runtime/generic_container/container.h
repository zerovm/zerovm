/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Generic containers.
 */

#ifndef NATIVE_CLIENT_GENERIC_CONTAINER_CONTAINER_H_
#define NATIVE_CLIENT_GENERIC_CONTAINER_CONTAINER_H_

#include <sys/types.h>

#include "native_client/src/include/nacl_base.h"

EXTERN_C_BEGIN

struct NaClCmpFunctorVtbl;

struct NaClCmpFunctor {
  struct NaClCmpFunctorVtbl *vtbl;
};

/**
 * OrderCmp should return 0 if objects are equal (so a 3-way compare
 * function that might be used for a tree container may be re-used
 * here).
 */
struct NaClCmpFunctorVtbl {
  void (*Dtor)(struct NaClCmpFunctor    *vself);
  int (*OrderCmp)(struct NaClCmpFunctor *vself,
                  void                  *left,
                  void                  *right);
};

struct NaClHashFunctorVtbl;

struct NaClHashFunctor {
  struct NaClHashFunctorVtbl const  *vtbl;
};

struct NaClHashFunctorVtbl {
  struct NaClCmpFunctorVtbl base;
  uintptr_t                 (*Hash)(struct NaClHashFunctor  *vself,
                                    void                    *datum);
};

struct NaClContainer;  /* fwd */

struct NaClContainerIter;  /* fwd */

struct NaClContainerVtbl {
  /*
   * Insert passes ownership of obj to the container; obj must be
   * malloc'd.
   */
  int                   (*Insert)(struct NaClContainer    *vself,
                                  void                    *obj);
  /*
   * Find: if returned/constructed iterator is not AtEnd, can Star it
   * to access datum, then Erase the entry to free it.  Erase
   * implicitly increments the iterator.  The out parameter should be
   * a pointer to an object of the appropriate subclass of
   * ContainerIter to be constructed.  It can be malloc'd/aligned
   * memory of iter_size bytes in size; the iterator will be placement
   * new'd in the memory.  All iterator objects are POD, so the dtor
   * is a no-op.
   */
  struct NaClContainerIter  *(*Find)(struct NaClContainer     *vself,
                                     void                     *key,
                                     struct NaClContainerIter *out);
  void                  (*Dtor)(struct NaClContainer          *vself);
  size_t                iter_size;
  int                   (*IterCtor)(struct NaClContainer      *vself,
                                    struct NaClContainerIter  *iter);
};

struct NaClContainer {
  struct NaClContainerVtbl const  *vtbl;
};

struct NaClContainerIterVtbl {
  int     (*AtEnd)(struct NaClContainerIter     *vself);
  void    *(*Star)(struct NaClContainerIter     *vself);
  void    (*Incr)(struct NaClContainerIter      *vself);
  void    (*Erase)(struct NaClContainerIter     *vself);
  void    *(*Extract)(struct NaClContainerIter  *vself);
  /* like erase, but takes ownership back, so no automatic free */
};

struct NaClContainerIter {
  struct NaClContainerIterVtbl const  *vtbl;
};

/* linked list */

struct NaClItemList {
  struct NaClItemList *next;
  void                *datum; /* dynamically allocated, pod */
};

struct NaClContainerList {
  struct NaClContainer  base;
  struct NaClCmpFunctor *cmp_functor;
  struct NaClItemList   *head;
};

int NaClContainerListCtor(struct NaClContainerList  *clp,
                          struct NaClCmpFunctor     *cmp_functor);

int NaClContainerListInsert(struct NaClContainer  *base_pointer,
                            void              *obj);

struct NaClContainerIter *NaClContainerListFind(
    struct NaClContainer      *base_pointer,
    void                      *key,
    struct NaClContainerIter  *out);

void NaClContainerListDtor(struct NaClContainer  *vself);

struct NaClContainerListIter {
  struct NaClContainerIter    base;
  struct NaClItemList         **cur;
};

int NaClContainerListIterCtor(struct NaClContainer      *vself,
                              struct NaClContainerIter  *viter);

/* hash table */

struct NaClContainerHashTblEntry {
  int   flags;
  void  *datum;
};

#define NACL_CHTE_USED       (1<<0)
/* slot occupied, so keep probing */

#define NACL_CHTE_DELETED    (1<<1)
/* slot occupied but deleted, keep probing */

struct NaClContainerHashTbl {
  struct NaClContainer              base;
  struct NaClHashFunctor            *hash_functor;
  size_t                            num_buckets;
  size_t                            num_entries;
  struct NaClContainerHashTblEntry  *bucket;
};

int NaClContainerHashTblCtor(struct NaClContainerHashTbl  *self,
                             struct NaClHashFunctor       *hash_functor,
                             size_t                       num_buckets);

int NaClContainerHashTblInsert(struct NaClContainer *vself,
                               void                 *obj);

struct NaClContainerIter *NaClContainerHashTblFind(
    struct NaClContainer      *vself,
    void                      *key,
    struct NaClContainerIter  *out);

void NaClContainerHashTblDtor(struct NaClContainer *vself);

struct NaClContainerHashTblIter {
  struct NaClContainerIter    base;
  struct NaClContainerHashTbl *htbl;
  uintptr_t                   idx;
};

int NaClContainerHashTblIterCtor(struct NaClContainer     *vself,
                                 struct NaClContainerIter *viter);


EXTERN_C_END

#endif
