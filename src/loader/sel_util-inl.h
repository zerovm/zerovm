/*
 * Copyright 2009 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
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
 * NaCl Simple/secure ELF loader (NaCl SEL) misc utilities.  Inlined
 * functions.  Internal; do not include.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_UTIL_INL_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_UTIL_INL_H_ 1

/*
 * NaClRoundPage is a bit of a misnomer -- it always rounds up to a
 * page size, not the nearest.
 */
static INLINE size_t  NaClRoundPage(size_t    nbytes) {
  return (nbytes + NACL_PAGESIZE - 1) & ~((size_t) NACL_PAGESIZE - 1);
}

static INLINE size_t  NaClRoundAllocPage(size_t    nbytes) {
  return (nbytes + NACL_MAP_PAGESIZE - 1) & ~((size_t) NACL_MAP_PAGESIZE - 1);
}

static INLINE size_t NaClTruncAllocPage(size_t  nbytes) {
  return nbytes & ~((size_t) NACL_MAP_PAGESIZE - 1);
}

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_UTIL_INL_H_ */
