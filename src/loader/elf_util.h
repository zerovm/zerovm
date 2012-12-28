/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ELF_UTIL_H__
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ELF_UTIL_H__ 1

#include "src/platform/portability.h"
#include "src/loader/sel_ldr.h"

struct NaClElfImage;
struct Gio;

uintptr_t NaClElfImageGetEntryPoint(struct NaClElfImage *image);

struct NaClElfImage *NaClElfImageNew(struct Gio *gp);

void NaClElfImageValidateElfHeader(struct NaClElfImage *image);

void NaClElfImageValidateProgramHeaders(
  struct NaClElfImage *image,
  uint8_t             addr_bits,
  uintptr_t           *static_text_end,
  uintptr_t           *rodata_start,
  uintptr_t           *rodata_end,
  uintptr_t           *data_start,
  uintptr_t           *data_end,
  uintptr_t           *max_vaddr);

/*
 * Loads an ELF executable before the address space's memory
 * protections have been set up by NaClMemoryProtection().
 */
void NaClElfImageLoad(struct NaClElfImage *image,
    struct Gio *gp, uint8_t addr_bits, uintptr_t mem_start);

void NaClElfImageDelete(struct NaClElfImage *image);


#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_ELF_UTIL_H__ */
