/*
 * replacement for the glibc library (not portable, should be used with gcc)
 *
 *  Created on: Feb 25, 2013
 *      Author: bortoq
 *
 * Copyright (C) 2002 Michael Ringgaard. All rights reserved.
 * (full (c) stuff see below)
 *
 * perhaps it would be wise to get rid of glibc usage completely.
 * by that and -nostdlib resulting nexe will contain no extra code
 * or fail to build.
 *
 * update: if NOSTDLIB defined library will not use standard
 *         functions (glibc)
 *
 * Copyright (C) 2002 Michael Ringgaard. All rights reserved.
 * (full (c) stuff see below)
 */
#define LONG_MAX 2147483647L
#define LONG_MIN (-LONG_MAX - 1L)
#define ULONG_MAX (LONG_MAX * 2UL + 1UL)

#include "api/zvm.h" /* todo: change it to zvm.h */
#include "zvmlib.h"
#include "bget.h"
#include "printf.h"

/*
 * zerovm library functions. some of functions are wrappers over
 * glibc ones. it is safe since they are not using nacl syscalls
 */

/* safe to use glibc version */
inline void *memset(void *s, int c, size_t n)
{
  unsigned char* p = s;

  while(n--)
    *p++ = (unsigned char)c;
  return s;
}

inline void *malloc(size_t size)
{
  return bget(size);
}

inline void *calloc(size_t n, size_t size)
{
  return bgetz((long long int)n * size);
}

inline void *realloc(void *p, size_t size)
{
  return bgetr(p, size);
}

inline void free(void *p)
{
  /* prevent crash */
  if(p == NULL) return;

  brel(p);
}

inline void *memcpy(void *dst, const void *src, size_t n)
{
  char* d = dst;
  char* s = (char*)src;
  --s;
  --d;

  while(n--) *++d = *++s;
  return dst;
}

/* safe to use glibc version */
inline int memcmp(const void *s1, const void *s2, size_t n)
{
  if(!n) return 0;
  while(--n && *(char*)s1 == *(char*)s2)
  {
    s1 = (char*)s1 + 1;
    s2 = (char*)s2 + 1;
  }
  return (*((unsigned char*)s1) - *((unsigned char*)s2));
}

inline char *strchr(const char *s, int c)
{
  while(*s != (char)c)
    if(!*s++)
      return 0;
  return (char *)s;
}

inline char *strrchr(const char *s, int c)
{
  char *ret = 0;

  do {
    if(*s == (char)c)
      ret = (char*)s;
  } while(*s++);

  return ret;
}

inline int strcmp(const void *s1, const void *s2)
{
  while(*(char*)s1 && (*(char*)s1 == *(char*)s2))
    s1++, s2++;
  return *(const unsigned char*) s1 - *(const unsigned char*) s2;
}

inline char *strcpy(char *dst, const char *src)
{
  char *ret = dst;
  while ((*dst++ = *src++))
    ;
  return ret;
}

inline char *strcat(char *dst, const char *src)
{
  char *ret = dst;
  while (*dst)
    dst++;
  while ((*dst++ = *src++))
    ;
  return ret;
}

/* safe to use glibc version */
inline size_t strlen(const char *s)
{
  const char *p = s;
  while(*s)
    ++s;
  return s - p;
}

inline size_t strspn(const char *s1, const char *s2)
{
  size_t ret = 0;

  while(*s1 && strchr(s2, *s1++))
    ret++;
  return ret;
}

inline size_t strcspn(const char *s1, const char *s2)
{
  size_t ret = 0;

  while(*s1)
    if(strchr(s2, *s1))
      return ret;
    else
      s1++, ret++;
  return ret;
}

inline char *strtok(char *str, const char *delim)
{
  static char* p = 0;

  if(str)
    p = str;
  else if(!p)
    return 0;

  str = p + strspn(p, delim);
  p = str + strcspn(str, delim);
  if(p == str)
    return p = 0;

  p = *p ? *p = 0, p + 1 : 0;
  return str;
}

inline int isupper(int c)
{
  return (c >= 'A' && c <= 'Z');
}

inline int islower(int c)
{
  return (c >= 'a' && c <= 'z');
}

inline int toupper(int c)
{
  return islower(c) ? c - 'a' + 'A' : c;
}

inline int isalpha(int c)
{
  return (isupper(c) || islower(c));
}

inline int isspace(int c)
{
  return (c == ' ' || c == '\t' || c == '\v'
      || c == '\f' || c == '\n' || c == '\r');
}

inline int isdigit(int c)
{
  return (c >= '0' && c <= '9');
}

#define FL_UNSIGNED   1
#define FL_NEG        2
#define FL_OVERFLOW   4
#define FL_READDIGIT  8
static unsigned long strtoxl(const char *nptr, char **endptr, int ibase, int flags)
{
  const unsigned char *p;
  char c;
  unsigned long number;
  unsigned digval;
  unsigned long maxval;

  p = (const unsigned char *) nptr;
  number = 0;

  c = *p++;
  while(isspace(c))
    c = *p++;

  if(c == '-')
  {
    flags |= FL_NEG;
    c = *p++;
  }
  else if(c == '+')
  {
    c = *p++;
  }

  if(ibase < 0 || ibase == 1 || ibase > 36)
  {
    if(endptr)
      *endptr = (char *) nptr;
    return 0L;
  }
  else if(ibase == 0)
  {
    if(c != '0')
    {
      ibase = 10;
    }
    else if(*p == 'x' || *p == 'X')
    {
      ibase = 16;
    }
    else
    {
      ibase = 8;
    }
  }

  if(ibase == 16)
  {
    if(c == '0' && (*p == 'x' || *p == 'X'))
    {
      ++p;
      c = *p++;
    }
  }

  maxval = ULONG_MAX / ibase;

  for(;;)
  {
    if(isdigit(c))
    {
      digval = c - '0';
    }
    else if(isalpha(c))
    {
      digval = toupper(c) - 'A' + 10;
    }
    else
    {
      break;
    }

    if(digval >= (unsigned) ibase)
      break;

    flags |= FL_READDIGIT;

    if(number < maxval || (number == maxval && (unsigned long) digval <= ULONG_MAX % ibase))
    {
      number = number * ibase + digval;
    }
    else
    {
      flags |= FL_OVERFLOW;
    }

    c = *p++;
  }

  --p;

  if(!(flags & FL_READDIGIT))
  {
    if(endptr)
      p = (const unsigned char*)nptr;
    number = 0;
  }
  else if((flags & FL_OVERFLOW)
      || (!(flags & FL_UNSIGNED)
          && (((flags & FL_NEG) && (number < LONG_MIN))
              || (!(flags & FL_NEG) && (number > LONG_MAX)))))
  {

    if(flags & FL_UNSIGNED)
    {
      number = ULONG_MAX;
    }
    else if(flags & FL_NEG)
    {
      number = LONG_MIN;
    }
    else
    {
      number = LONG_MAX;
    }
  }

  if(endptr)
    *endptr = (char *) p;

  if(flags & FL_NEG)
    number = (unsigned long) (-(long) number);

  return number;
}

inline long int strtol(const char *p, char **end, int base)
{
  return (long) strtoxl(p, end, base, 0);
}

inline unsigned long strtoul(const char *p, char **end, int base)
{
  return strtoxl(p, end, base, FL_UNSIGNED);
}

inline long atol(const char *nptr)
{
  int c;
  int sign;
  long total;
  const unsigned char *p = (const unsigned char *) nptr;

  while(isspace(*p))
    ++p;

  c = *p++;
  sign = c;
  if(c == '-' || c == '+')
    c = *p++;

  total = 0;
  while(isdigit(c))
  {
    total = 10 * total + (c - '0');
    c = *p++;
  }

  if(sign == '-')
  {
    return -total;
  }
  else
  {
    return total;
  }
}

inline int atoi(const char *s)
{
  return (int)atol(s);
}

static unsigned long int seed = 1;
inline void srand(unsigned int s)
{
  seed = s;
}

inline int rand()
{
  seed = seed * 1103515245 + 12345;
  return (unsigned int)(seed/65536) % 32768;
}

/*
 * get handle by the channel alias. return -1 if failed
 * todo: replace it with open(), change dependent functions
 */
inline int handle(const char *alias)
{
  if(alias != NULL && *alias != '\0')
  {
    int i;
    for(i = 0; i < MANIFEST->channels_count; ++i)
      if(STRCMP(MANIFEST->channels[i].name, alias) == 0)
        return i;
  }

  return -1;
}

#if 0
/* ### {{ */
#define EOL "\r\n"
#define ENVIRONMENT "/dev/nvram"
#define ENVIRONMENT_MAX_SIZE 0x10000
uint32_t *get_start_info()
{
//  char buffer[ENVIRONMENT_MAX_SIZE];
  char *buffer = NULL;
  char *strings = NULL;
  uint32_t *info = NULL;
  char *strings_start = NULL;
  int handle = -1;
  int i; /* indices of info and strings */

  /* find "/dev/nvram" handle */
  for(i = 3; i < MANIFEST->channels_count; ++i)
  {
    if(strcmp(MANIFEST->channels[i].name, ENVIRONMENT) == 0)
    {
      handle = i;
      break;
    }
  }
  if(handle < 0) goto fail;

  /* allocate temporary buffer */
  buffer = malloc(ENVIRONMENT_MAX_SIZE);
  if(buffer == NULL) goto fail;

  /* read nvram content and ensure the size */
  i = zvm_pread(handle, buffer, ENVIRONMENT_MAX_SIZE, 0);
  if(i == ENVIRONMENT_MAX_SIZE) goto fail;

  /* allocate area to hold nvram information */
  info = calloc(i, sizeof strings);
  strings = calloc(i, sizeof *strings);
  if(info == NULL || strings == NULL) goto fail;
  strings_start = strings;

  /* parse command line arguments */
  buffer = strtok(buffer, EOL);
  for(i = 3; buffer; ++i)
  {
    strcpy(strings, buffer);
    info[i] = (uintptr_t)strings;
    strings += strlen(buffer) + 1;

    buffer = strtok(NULL, EOL);
  }
  info[2] = i;
  info[i] = 0;

  /* todo: parse environment */

  /* adjust buffers size */
  strings = realloc(strings, strings - strings_start);
  info = realloc(info, i * sizeof *info);
  return info;

  fail:
  free(buffer);
  free(strings);
  free(info);
  return NULL;
}
#endif
/* }} */

/* initializer of zlib (replaces glibc prologue) */
static void _init()
{
  /* setup memory manager */
  bpool(MANIFEST->heap_ptr, MANIFEST->heap_size);
}

/* the standard glibc prologue replacement */
int main(int argc, char **argv, char **envp);
void _start(uint32_t *info)
{
  int argc = info[2];
  char **argv = (void*)&info[3];
  char **envp = argv + argc + 1;
//  int argc;
//  char **argv;
//  char **envp;
//  uint32_t *p;

  /*
   * if the start information is available through the "/dev/nvram"
   * channel, use it instead of the given argument
   */
//  p = get_start_info();
//  if(info != NULL) info = p;
//
//  /* set the main() arguments */
//  argc = info[2];
//  argv = (void*)&info[3];
//  envp = argv + argc + 1;

  /* put marker to the bottom of the stack */
  *(uint64_t*)0xFF000000 = 0x716f74726f622764LLU;

  /* initialize zerovm library */
  _init();

  /* call the user main and exit to zerovm */
  zvm_exit(main(argc, argv, envp));
}

/*
 * strtol.c
 *
 * String to number conversion
 *
 * Copyright (C) 2002 Michael Ringgaard. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
