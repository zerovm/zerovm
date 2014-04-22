/*
 * replacement for the glibc library (not portable, should be used with gcc)
 *
 * perhaps it would be wise to get rid of glibc usage completely.
 * by that and -nostdlib resulting nexe will contain no extra code
 * or fail to build.
 *
 * update: if NOSTDLIB defined library will not use standard
 *         functions (glibc)
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
/*
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

/* get command line and environment data from the channel */
static int get_nvram_data(char *buffer)
{
  int i = OPEN(ENVIRONMENT);

  if(i >= 0)
    i = zvm_pread(i, buffer, ENVIRONMENT_MAX_SIZE, 0);
  return i == ENVIRONMENT_MAX_SIZE || i < 1 ? -1 : i;
}

#define FAILIF(cond) if(cond) {free(strings); free(info); return (void*)0xFFFFFFD8;}
#define PARSE(buf) \
  for(buffer = strtok(buf, EOL); buffer; ++i)\
  {\
    /* check for end of section */\
    if(strcmp(buffer, END_OF_SECTION) == 0) break;\
    strcpy(strings, buffer);\
    info[i] = (uintptr_t)strings;\
    strings += strlen(buffer) + 1; /* +1 for '\0' */\
    buffer = strtok(NULL, EOL);\
  }

uint32_t *get_start_info()
{
  char buf[ENVIRONMENT_MAX_SIZE], *buffer = buf;
  char *strings = NULL;
  uint32_t *info = NULL;
  int i;

  /* get the data */
  i = get_nvram_data(buffer);
  FAILIF(i <= 0);

  /* allocate area to hold nvram information */
  info = calloc(i, sizeof strings);
  strings = calloc(i, sizeof *strings);
  FAILIF(info == NULL || strings == NULL);

  /* parse command line arguments */
  i = 1;
  PARSE(buffer);
  info[i] = 0;
  info[0] = i - 1;

  /* parse environment */
  ++i;
  PARSE(NULL);

  /* todo: buffers sizes adjustment disabled since bget reallocate is lame */
  return info;
}
#undef PARSE
#undef FAILIF

/* initializer of zlib (replaces glibc prologue) */
static void _init()
{
  /* setup memory manager */
  bpool(MANIFEST->heap_ptr, MANIFEST->heap_size);
}

/* the standard glibc prologue replacement */
/*
 * the standard glibc prologue replacement. command line arguments
 * and environment will be read from the "/dev/nvram" channel instead
 * of "info" passed as _start argument
 */
int main(int argc, char **argv, char **envp);
void _start(uint32_t *info)
{
  /* initialize zerovm library */
  _init();

  uint32_t *p = get_start_info();
  int argc = p[0];
  char **argv = (void*)&p[1];
  char **envp = (void*)&p[argc + 2];

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
