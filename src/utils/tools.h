/*
 * all macros, definitions and small functions which
 * not fit to other header and often used are here
 *
 *  Created on: Jun 17, 2012
 *      Author: d'b
 */

#ifndef TOOLS_H_
#define TOOLS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "src/service_runtime/include/sys/errno.h"

#define KEYWORD_SIZE_MAX 256
#define BIG_ENOUGH_SPACE 65536 /* ..size of the biggest temporary variable */

/* check condition 'cond' if false aborts zerovm with message 'msg' */
/*
 * check condition 'cond' if false aborts zerovm with message 'msg'
 * WARNING: must not be defined as an empty macro! it used to run functions
 */
#define COND_ABORT(cond, msg) if(cond) {fprintf(stderr, "%s\n", msg); exit(1);}

/* malloc and check. if allocation failed abort with message 'msg' */
static inline void *my_malloc(size_t size, const char *msg)
{
  void *ptr = malloc(size);
  COND_ABORT(ptr == NULL, msg);
  return ptr;
}
#define MALLOC(size, msg) my_malloc((size), (msg))

/* safe strlen(). NULL can be used. return 0 for NULL */
static inline size_t my_strlen(const char *str)
{
  return (str == NULL) ? 0 : strlen(str);
}
#define STRLEN(str) my_strlen(str)

/*
 * safe atoi(). NULL can be used. return 0 for NULL
 * todo(d'b): c99 function used. should be replaced with c90 code
 */
static inline int64_t my_atoi(const char *str)
{
  return (str == NULL) ? 0 : (int64_t)strtoll(str, NULL, 10);
}
#define ATOI(str) my_atoi(str)

/* safe strcpy(). NULL can be used. return NULL if dst or src is NULL */
static inline void *my_strcpy(char *dst, const char *src)
{
  return (src == NULL || dst == NULL) ? NULL : strcpy(dst, src);
}
#define STRCPY(dst, src) my_strcpy((dst), (src))

/* safe strncpy(). NULL can be used. return NULL if dst or src is NULL */
static inline void *my_strncpy(char *dst, const char *src, size_t size)
{
  return (src == NULL || dst == NULL) ? NULL : strncpy(dst, src, size);
}
#define STRNCPY(dst, src) my_strncpy((dst), (src))

/* safe memcpy(). NULL can be used. return NULL if copy failed */
static inline void *my_memcpy(void *dst, const void *src, size_t size)
{
  return (dst == NULL || src == NULL) ? NULL : memcpy(dst, src, size);
}
#define MEMCPY(dst, src, cnt) my_memcpy((dst), (src), (cnt))

/* safe strcmp(). NULL can be used. return -1 if failed */
static inline int my_strcmp(const char *a, const char *b)
{
  return (a == NULL || b == NULL) ? -1 : strcmp(a, b);
}
#define STRCMP(a, b) my_strcmp((a), (b))

/* safe streq(). NULL can be used. return -1 if failed */
static inline int my_streq(const char *a, const char *b)
{
  return STRCMP(a, b) == 0;
}
#define STREQ(a, b) my_streq((a), (b))

/* safe strneq(). NULL can be used. return -1 if failed */
static inline int my_strneq(const char *a, const char *b)
{
  return !STREQ(a, b);
}
#define STRNEQ(a, b) my_strneq((a), (b))

/* return size of given file or negative error code */
static inline int64_t GetFileSize(const char *name)
{
  struct stat fs;
  int handle = open(name, O_RDONLY);
  return fstat(handle, &fs), close(handle) ? -NACL_ABI_EPERM : fs.st_size;
}

#endif /* TOOLS_H_ */
