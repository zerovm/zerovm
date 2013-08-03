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

#include <sys/mman.h>
#include "src/main/zlog.h"

int NaCl_page_alloc_intern_flags(void **p, size_t size, int map_flags)
{
  void *addr;

  map_flags |= MAP_PRIVATE | MAP_ANONYMOUS;

  ZLOGS(LOG_INSANE, "mmap(%p, %lX, %#x, %#x, %d, %d)", *p, size, PROT_NONE, map_flags, -1, 0);

  addr = mmap(*p, size, PROT_NONE, map_flags, -1, (off_t)0);
  if(MAP_FAILED == addr) addr = NULL;
  if(NULL != addr) *p = addr;
  return (NULL == addr) ? -ENOMEM : 0;
}

int NaCl_page_free(void *p, size_t size)
{
  int ret = munmap(p, size);

  return ret == -1 ? -errno : ret;
}

int NaCl_mprotect(void *addr, size_t len, int prot)
{
  int ret = mprotect(addr, len, prot);

  return ret == -1 ? -errno : ret;
}

int NaCl_madvise(void *start, size_t length, int advice)
{
  int ret = madvise(start, length, advice);

  /* MADV_DONTNEED and MADV_NORMAL are needed */
  return ret == -1 ? -errno : ret;
}
