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
 *
 * notice: "NaCl" is here by historical reasons. in fact zvm only can
 * use binaries built with ZeroVM toolchain (former NaCl toolchain)
 *
 * This loader can only process ZeroVM object files as produced using
 * the ZeroVM toolchain.  Other ELF files will be rejected.
 *
 * The primary function, NaClAppLoadFile, parses an ELF file,
 * allocates memory, loads the relocatable image from the ELF file
 * into memory, and performs relocation.  NaClAppRun runs the
 * resultant program.
 */

#ifndef SEL_LDR_H_
#define SEL_LDR_H_ 1

#include <stdint.h>

#include "src/main/config.h"
#include "src/loader/sel_rt.h"
#include "src/main/manifest.h"

EXTERN_C_BEGIN

struct NaClApp {
  /* only used for ET_EXEC:  for CS restriction */
  uintptr_t                 static_text_end;  /* relative to mem_start */
  /* ro after app starts. memsz from phdr */

  /*
   * rodata_start and data_start may be 0 if these segments are not
   * present in the executable.
   */
  uintptr_t                 rodata_start;  /* initialized data, ro */
  uintptr_t                 data_start;    /* initialized data/bss, rw */
  /*
   * Various region sizes must be a multiple of NACL_MAP_PAGESIZE
   * before the NaCl app can run.  The sizes from the ELF file
   * (p_filesz field) might not be -- that would waste space for
   * padding -- and while we could use p_memsz to specify padding, but
   * we will record the virtual addresses of the start of the segments
   * and figure out the gap between the p_vaddr + p_filesz of one
   * segment and p_vaddr of the next to determine padding.
   */

  uintptr_t                 data_end;
  /* see break_addr below */

  /*
   * initial_entry_pt is the first address in untrusted code to jump
   * to. initial_entry_pt is in the user executable.
   */
  uintptr_t                 initial_entry_pt;

  uintptr_t                 break_addr; /* user addr */
  /* data_end <= break_addr is an invariant */

  struct Manifest           *manifest;
};

/* global variables */
struct ThreadContext    *nacl_user; /* user registers storage */
struct ThreadContext    *nacl_sys;  /* zerovm registers storage */
struct NaClApp              *gnap;  /* pointer to global NaClApp object */

#include "src/loader/sel_ldr-inl.h"

void ThreadContextCtor(struct ThreadContext *ntcp,
                       nacl_reg_t            prog_ctr,
                       nacl_reg_t            stack_ptr);

EXTERN_C_END

#endif  /* SEL_LDR_H_ */
