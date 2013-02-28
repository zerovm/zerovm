/*
 *  Created on: Feb 25, 2013
 *      Author: bortoq
 *
 * Copyright (C) 2002 Michael Ringgaard. All rights reserved.
 * (full (c) stuff see below)
 */
#include <stdio.h>
#include <limits.h>
#include <string.h> /* todo: remove it after wrappers will be removed */
#include "api/zvm.h" /* todo: change it to zvm.h */
#include "zvmlib.h"
#include "bget.h"
#include "printf.h"

/*
 * zerovm library functions. some of functions are wrappers over
 * glibc ones. it is safe since they are not using nacl syscalls
 */

/* safe to use glibc version */
void *zl_memset(void *s, int c, size_t n)
{
  return memset(s, c, n);
}

void *zl_malloc(size_t size)
{
  return bget(size);
}

void *zl_calloc(size_t n, size_t size)
{
  return bgetz((long long int)n * size);
}

void *zl_realloc(void *p, size_t size)
{
  return bgetr(p, size);
}

void zl_free(void *p)
{
  /* prevent crash */
  if(p == NULL) return;

  brel(p);
}

void *zl_memcpy(void *dst, const void *src, size_t n)
{
  char* d = dst;
  char* s = (char*)src;
  --s;
  --d;

  while(n--) *++d = *++s;
  return dst;
}

/* safe to use glibc version */
int zl_memcmp(const void *s1, const void *s2, size_t n)
{
  return memcmp(s1, s2, n);
}

/* ### */
char *zl_strchr(const char *s, int c)
{
  return strchr(s, c);
}

/* ### */
char *zl_strrchr(const char *s, int c)
{
  return strchr(s, c);
}

/* safe to use glibc version */
int zl_strcmp(const void *s1, const void *s2)
{
  return strcmp(s1, s2);
}

/* safe to use glibc version */
size_t zl_strlen(const char *s)
{
  return strlen(s);
}

/* safe to use glibc version */
char *zl_strtok(char *s, const char *delim)
{
  return strtok(s, delim);
}

int zl_isupper(char c)
{
  return (c >= 'A' && c <= 'Z');
}

int zl_islower(char c)
{
  return (c >= 'a' && c <= 'z');
}

static int zl_toupper(char c)
{
  return zl_islower(c) ? c - 'a' + 'A' : c;
}

int zl_isalpha(char c)
{
  return (zl_isupper(c) || zl_islower(c));
}

int zl_isspace(char c)
{
  return (c == ' ' || c == '\t' || c == '\v'
      || c == '\f' || c == '\n' || c == '\r');
}

int zl_isdigit(char c)
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
  while(zl_isspace(c))
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
    if(zl_isdigit(c))
    {
      digval = c - '0';
    }
    else if(zl_isalpha(c))
    {
      digval = zl_toupper(c) - 'A' + 10;
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

long int zl_strtol(const char *p, char **end, int base)
{
  return (long) strtoxl(p, end, base, 0);
}

unsigned long zl_strtoul(const char *p, char **end, int base)
{
  return strtoxl(p, end, base, FL_UNSIGNED);
}

long atol(const char *nptr)
{
  int c;
  int sign;
  long total;
  const unsigned char *p = (const unsigned char *) nptr;

  while(zl_isspace(*p))
    ++p;

  c = *p++;
  sign = c;
  if(c == '-' || c == '+')
    c = *p++;

  total = 0;
  while(zl_isdigit(c))
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

int zl_atoi(const char *s)
{
  return (int) atol(s);
}

static unsigned long int seed = 1;
void zl_srand(unsigned long int s)
{
  seed = s;
}

int zl_rand()
{
  seed = seed * 1103515245 + 12345;
  return (unsigned int)(seed/65536) % 32768;
}

/*
 * get handle by the channel alias. return -1 if failed
 * todo: replace it with open(), change dependent functions
 */
int zl_handle(const char *alias)
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

/* initializer of zlib (replaces glibc prologue) */
static void zl_init()
{
  /* setup memory manager */
  bpool(MANIFEST->heap_ptr, MANIFEST->heap_size);
}

/* the standard glibc prologue replacement */
void __libc_start_main (
  int (*main) (int argc, char **argv, char **envp),
  int argc,
  char **argv,
  int (*init) (int argc, char **argv, char **envp),
  void (*fini) (void),
  void (*rtld_fini) (void),
  void *stack_end
  )
{
  int code;

  /* initialize zerovm library */
  zl_init();

  /* call the user main */
  code = main(argc, argv, NULL);

  /* exit to zerovm */
  zvm_exit(code);
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
