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

#ifndef ELF_UTIL_H__
#define ELF_UTIL_H__ 1

#include "src/platform/portability.h"
#include "src/loader/sel_ldr.h"

struct NaClElfImage;
struct Gio;

uintptr_t NaClElfImageGetEntryPoint(const struct NaClElfImage *image);

struct NaClElfImage *NaClElfImageNew(struct Gio *gp);

void NaClElfImageValidateElfHeader(const struct NaClElfImage *image);

/*
 * TODO(robertm): decouple validation from computation of static_text_end
 * and max_vaddr
 * todo(d'b): the function is too large, rewrite it
 */
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
void NaClElfImageLoad(const struct NaClElfImage *image,
    struct Gio *gp, uint8_t addr_bits, uintptr_t mem_start);

void NaClElfImageDelete(struct NaClElfImage *image);

#endif  /* ELF_UTIL_H__ */
