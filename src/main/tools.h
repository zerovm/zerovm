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

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#include <unistd.h>
#include <sys/stat.h>

/*
 * putting extern "C" { } in header files make emacs want to indent
 * everything, which looks odd.  rather than putting in fancy syntax
 * recognition in c-mode, we just use the following macros.
 */
#ifdef __cplusplus
# define EXTERN_C_BEGIN  extern "C" {
# define EXTERN_C_END    }

/* Mark this function as not throwing beyond */
#else
# define EXTERN_C_BEGIN
# define EXTERN_C_END
#endif

/* gcc only */
#define INLINE __inline__
#define UNREFERENCED_PARAMETER(P) do { (void) P; } while (0)
#define NORETURN __attribute__((noreturn))
#define NACL_WUR __attribute__((__warn_unused_result__))

/* size of the random biggest string */
#define BIG_ENOUGH_STRING 1024
#define MICROS_PER_MILLI 1000

/* debug macro */
#undef SHOWID
#define SHOWID printf("%s: %d, %s\n", __FILE__, __LINE__, __func__)

#define ROUNDDOWN_64K(a) ((a) & ~(NACL_MAP_PAGESIZE - 1LLU))
#define ROUNDUP_64K(a) ROUNDDOWN_64K((a) + NACL_MAP_PAGESIZE - 1LLU)
#define ROUNDDOWN_4K(a) ((a) & ~(NACL_PAGESIZE - 1LLU))
#define ROUNDUP_4K(a) ROUNDDOWN_4K((a) + NACL_PAGESIZE - 1LLU)

/* safe atoi(). return 0 for NULL */
#define ATOI(str) ((str == NULL) ? 0 : g_ascii_strtoll(str, NULL, 0))

/* return size of given file or negative error code */
int64_t GetFileSize(const char *name);

#endif /* TOOLS_H_ */
