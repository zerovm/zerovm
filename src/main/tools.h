/*
 * all macros, definitions and small functions which
 * not fit to other header and often used are here
 *
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

#ifndef TOOLS_H_
#define TOOLS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "src/main/nacl_base.h"
#include "src/main/nacl_exit.h"
#include "src/main/zlog.h"

#define MANIFEST_MAX 0x100000 /* limit for the manifest size */
#define KEYWORD_SIZE_MAX 256
#define BIG_ENOUGH_SPACE 0x10000 /* ..size of the biggest stack variable */
#define BIG_ENOUGH_STRING 1024 /* ..size of the random biggest string */
#define INT32_STRLEN (11) /* enough space to place maximum int32 value + '\0' */

/* debug macro */
#undef SHOWID
#define SHOWID printf("%s: %d, %s\n", __FILE__, __LINE__, __func__)

#define ROUNDDOWN_64K(a) ((a) & ~(NACL_MAP_PAGESIZE - 1LLU))
#define ROUNDUP_64K(a) ROUNDDOWN_64K((a) + NACL_MAP_PAGESIZE - 1LLU)
#define ROUNDDOWN_4K(a) ((a) & ~(NACL_PAGESIZE - 1LLU))
#define ROUNDUP_4K(a) ROUNDDOWN_4K((a) + NACL_PAGESIZE - 1LLU)

/* safe atoi(). NULL can be used. return 0 for NULL */
static INLINE int64_t safe_atoi(const char *str)
{
  return (str == NULL) ? 0 : g_ascii_strtoll(str, NULL, 10);
}
#define ATOI(str) safe_atoi(str)

/* safe streq(). NULL can be used. return 1 when a == b */
static INLINE int safe_streq(const char *a, const char *b)
{
  return g_strcmp0(a, b) == 0;
}
#define STREQ(a, b) safe_streq((a), (b))

/* return size of given file or negative error code */
static INLINE int64_t GetFileSize(const char *name)
{
  struct stat fs;
  int handle = open(name, O_RDONLY);
  return fstat(handle, &fs), close(handle) ? -1 : fs.st_size;
}

#endif /* TOOLS_H_ */
