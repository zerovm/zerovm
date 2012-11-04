/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * Implementation of dynamic arrays.
 */

#include <string.h>
#include "src/service_runtime/dyn_array.h"

static int const kBitsPerWord = 32;
static int const kWordIndexShift = 5;  /* 2**kWordIndexShift==kBitsPerWord */

static INLINE size_t BitsToAllocWords(size_t nbits) {
  return (nbits + kBitsPerWord - 1) >> kWordIndexShift;
}

int DynArrayCtor(struct DynArray  *dap,
                 size_t           initial_size) {
  if (initial_size == 0) {
    initial_size = 32;
  }
  dap->num_entries = 0u;
  /* calloc should check internally, but we're paranoid */
  if (SIZE_T_MAX / sizeof *dap->ptr_array < initial_size ||
      SIZE_T_MAX/ sizeof *dap->available < BitsToAllocWords(initial_size)) {
    /* would integer overflow */
    return 0;
  }
  dap->ptr_array = calloc(initial_size, sizeof *dap->ptr_array);
  if (NULL == dap->ptr_array) {
    return 0;
  }
  dap->available = calloc(BitsToAllocWords(initial_size),
                          sizeof *dap->available);
  if (NULL == dap->available) {
    free(dap->ptr_array);
    dap->ptr_array = NULL;
    return 0;
  }
  dap->avail_ix = 0;  /* hint */

  dap->ptr_array_space = initial_size;
  return 1;
}

void DynArrayDtor(struct DynArray *dap) {
  dap->num_entries = 0;  /* assume user has freed entries */
  free(dap->ptr_array);
  dap->ptr_array = NULL;
  dap->ptr_array_space = 0;
  free(dap->available);
  dap->available = NULL;
}
