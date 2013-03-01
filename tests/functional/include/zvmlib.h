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

#define BIG_ENOUGH 0x10000

/* todo: remove it */
void tfp_sprintf(char *s, char *fmt, ...);

/***************************************************
 * zero library helper macros
 ***************************************************/
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/***************************************************
 * zero library functionality available via macros
 ***************************************************/
#define MALLOC(size) zl_malloc(size)
#define CALLOC(n, size) zl_calloc(n, size)
#define REALLOC(p, size) zl_realloc(p, size)
#define FREE(p) zl_free(p)
#define MEMSET(s, c, n) zl_memset(s, c, n)
#define MEMCPY(dst, src, n) zl_memcpy(dst, src, n)
#define MEMCMP(s1, s2, n) zl_memcmp(s1, s2, n)
#define ISUPPER(c) zl_isupper(c)
#define ISLOWER(c) zl_islower(c)
#define ISALPHA(c) zl_isalpha(c)
#define ISSPACE(c) zl_isspace(c)
#define ISDIGIT(c) zl_isdigit(c)
#define STRCHR(s, c) zl_strchr(s, c)
#define STRRCHR(s, c) zl_strchr(s, c)
#define STRSPN(s1, s2) zl_strspn(s1, s2)
#define STRCSPN(s1, s2) zl_strcspn(s1, s2)
#define STRCMP(s1, s2) zl_strcmp(s1, s2)
#define STRLEN(s) zl_strlen(s)
#define STRTOK(s, delim) zl_strtok(s, delim)
#define ATOI(s) zl_atoi(s)
#define STRTOL(p, end, base) zl_strtol(p, end, base)
#define STRTOUL(p, end, base) zl_strtoul(p, end, base)
#define RAND() zl_rand()
#define SRAND(s) zl_rand(s)
#define OPEN(alias) zl_handle(alias) /* will be fixed */
#define CLOSE(handle)
#define READ(alias, buf, size) zvm_pread(zl_handle(alias), buf, size, 0)
#define WRITE(alias, buf, size) zvm_pwrite(zl_handle(alias), buf, size, 0)
#define PREAD(alias, buf, size, offset) zvm_pread(zl_handle(alias), buf, size, offset)
#define PWRITE(alias, buf, size, offset) zvm_pwrite(zl_handle(alias), buf, size, offset)
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

/***************************************************
 * zero library functions. it is recommended to use
 * above macros instead
 ***************************************************/
void *zl_malloc(size_t size);
void *zl_calloc(size_t n, size_t size);
void *zl_realloc(void *p, size_t size);
void zl_free(void *p);
void *zl_memset(void *s, int c, size_t n);
void *zl_memcpy(void *dst, const void *src, size_t n);
int zl_memcmp(const void *s1, const void *s2, size_t n);
int zl_isupper(char c);
int zl_islower(char c);
int zl_isalpha(char c);
int zl_isspace(char c);
int zl_isdigit(char c);
char *zl_strchr(const char *s, int c);
char *zl_strrchr(const char *s, int c);
size_t zl_strspn(const char *s1, const char *s2);
size_t zl_strcspn(const char *s1, const char *s2);
int zl_strcmp(const void *s1, const void *s2);
size_t zl_strlen(const char *s);
char *zl_strtok(char *s, const char *delim);
int zl_atoi(const char *s);
long int zl_strtol(const char *p, char **end, int base);
unsigned long zl_strtoul(const char *p, char **end, int base);
int zl_rand();
void zl_srand(unsigned long int s);
int zl_handle(const char *alias);

#endif /* ZLIB_H_ */
