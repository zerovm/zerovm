/*
 * zero library is a small set of functions to replace standard library
 * (zvm api need to be tested w/o glibc)
 *
 *  Created on: Feb 25, 2013
 *      Author: bortoq
 */

#ifndef ZLIB_H_
#define ZLIB_H_

#include <stdint.h>
#include <stdio.h> /* todo: get rid of it */
#include "api/zvm.h" /* zerovm api */

/***************************************************
 * zlib constants, macros and types
 ***************************************************/

/* standart channels */
#define STDIN "/dev/stdin"
#define STDOUT "/dev/stdout"
#define STDERR "/dev/stderr"

#define EOL "\r\n"
#define END_OF_SECTION ";"
#define ENVIRONMENT "/dev/nvram"
#define ENVIRONMENT_MAX_SIZE 0x1000

#define BIG_ENOUGH 0x10000

/***************************************************
 * zero library helper macros
 ***************************************************/
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/***************************************************
 * zero library functionality available via macros
 ***************************************************/
#define MALLOC(size) malloc(size)
#define CALLOC(n, size) calloc(n, size)
#define REALLOC(p, size) realloc(p, size)
#define FREE(p) free(p)
#define MEMSET(s, c, n) memset(s, c, n)
#define MEMCPY(dst, src, n) memcpy(dst, src, n)
#define MEMCMP(s1, s2, n) memcmp(s1, s2, n)
#define ISUPPER(c) isupper(c)
#define TOUPPER(c) toupper(c)
#define ISLOWER(c) islower(c)
#define ISALPHA(c) isalpha(c)
#define ISSPACE(c) isspace(c)
#define ISDIGIT(c) isdigit(c)
#define STRCHR(s, c) strchr(s, c)
#define STRRCHR(s, c) strchr(s, c)
#define STRSPN(s1, s2) strspn(s1, s2)
#define STRCSPN(s1, s2) strcspn(s1, s2)
#define STRCPY(dst, src) strcpy(dst, src)
#define STRCAT(dst, src) strcat(dst, src)
#define STRCMP(s1, s2) strcmp(s1, s2)
#define STRLEN(s) strlen(s)
#define STRTOK(s, delim) strtok(s, delim)
#define ATOI(s) atoi(s)
#define STRTOL(p, end, base) strtol(p, end, base)
#define STRTOUL(p, end, base) strtoul(p, end, base)
#define RAND() rand()
#define SRAND(s) srand(s)
#define OPEN(alias) handle(alias) /* will be fixed */
#define CLOSE(handle)
#define EXIT(code) zvm_exit(code)
#define exit(code) zvm_exit(code)
#define READ(alias, buf, size) zvm_pread(handle(alias), buf, size, 0)
#define WRITE(alias, buf, size) zvm_pwrite(handle(alias), buf, size, 0)
#define PREAD(alias, buf, size, offset) zvm_pread(handle(alias), buf, size, offset)
#define PWRITE(alias, buf, size, offset) zvm_pwrite(handle(alias), buf, size, offset)
#define SPRINTF tfp_sprintf
#define FPRINTF(alias, ...) \
  do {\
    char _buf[BIG_ENOUGH];\
    int _size;\
    SPRINTF(_buf, __VA_ARGS__);\
    _size = STRLEN(_buf);\
    WRITE(alias, _buf, _size);\
  } while(0)
#define PRINTF(...) FPRINTF(STDOUT, __VA_ARGS__)
#define fprintf FPRINTF
#define printf PRINTF
#define sprintf SPRINTF

/***************************************************
 * zero library functions. it is recommended to use
 * above macros instead
 ***************************************************/
inline void *malloc(size_t size);
inline void *calloc(size_t n, size_t size);
inline void *realloc(void *p, size_t size);
inline void free(void *p);
inline void *memset(void *s, int c, size_t n);
inline void *memcpy(void *dst, const void *src, size_t n);
inline int memcmp(const void *s1, const void *s2, size_t n);
inline int isupper(int c);
inline int toupper(int c);
inline int islower(int c);
inline int isalpha(int c);
inline int isspace(int c);
inline int isdigit(int c);
inline char *strchr(const char *s, int c);
inline char *strrchr(const char *s, int c);
inline size_t strspn(const char *s1, const char *s2);
inline size_t strcspn(const char *s1, const char *s2);
inline char *strcpy(char *dst, const char *src);
inline char *strcat(char *dst, const char *src);
inline int strcmp(const void *s1, const void *s2);
inline size_t strlen(const char *s);
inline char *strtok(char *s, const char *delim);
inline int atoi(const char *s);
inline long int strtol(const char *p, char **end, int base);
inline unsigned long strtoul(const char *p, char **end, int base);
inline int rand();
inline void srand(unsigned int s);
inline int handle(const char *alias);
void tfp_sprintf(char *s, char *fmt, ...);

#endif /* ZLIB_H_ */
