/*
 File: printf.c

 Copyright (C) 2004  Kustaa Nyholm

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "printf.h"

typedef void (*putcf)(void *, char);
static putcf stdout_putf;
static void *stdout_putp;

struct param
{
  char lz; /**< Leading zeros */
  unsigned int width; /**< field width */
  char sign; /**<  The sign to display (if any) */
  char alt; /**< alternate form */
  unsigned int base; /**<  number base (e.g.: 8, 10, 16) */
  char uc; /**<  Upper case (for base16 only) */
  char *bf; /**<  Buffer to output */
};

#ifdef PRINTF_LONG_SUPPORT

static void uli2a(unsigned long int num, struct param *p)
{
  int n = 0;
  unsigned long int d = 1;
  char *bf = p->bf;
  while(num / d >= p->base)
    d *= p->base;
  while(d != 0)
  {
    int dgt = num / d;
    num %= d;
    d /= p->base;
    if(n || dgt > 0 || d == 0)
    {
      *bf++ = dgt + (dgt < 10 ? '0' : (p->uc ? 'A' : 'a') - 10);
      ++n;
    }
  }
  *bf = 0;
}

static void li2a(long num, struct param *p)
{
  if(num < 0)
  {
    num = -num;
    p->sign = '-';
  }
  uli2a(num, p);
}
#endif

static void ui2a(unsigned int num, struct param *p)
{
  int n = 0;
  unsigned int d = 1;
  char *bf = p->bf;
  while(num / d >= p->base)
    d *= p->base;
  while(d != 0)
  {
    int dgt = num / d;
    num %= d;
    d /= p->base;
    if(n || dgt > 0 || d == 0)
    {
      *bf++ = dgt + (dgt < 10 ? '0' : (p->uc ? 'A' : 'a') - 10);
      ++n;
    }
  }
  *bf = 0;
}

static void i2a(int num, struct param *p)
{
  if(num < 0)
  {
    num = -num;
    p->sign = '-';
  }
  ui2a(num, p);
}

static int a2d(char ch)
{
  if(ch >= '0' && ch <= '9')
    return ch - '0';
  else if(ch >= 'a' && ch <= 'f')
    return ch - 'a' + 10;
  else if(ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;
  else
    return -1;
}

static char a2i(char ch, char **src, int base, int *nump)
{
  char *p = *src;
  int num = 0;
  int digit;
  while((digit = a2d(ch)) >= 0)
  {
    if(digit > base)
      break;
    num = num * base + digit;
    ch = *p++;
  }
  *src = p;
  *nump = num;
  return ch;
}

static void putchw(void *putp, putcf putf, struct param *p)
{
  char ch;
  int n = p->width;
  char *bf = p->bf;

  /* Number of filling characters */
  while(*bf++ && n > 0)
    n--;
  if(p->sign)
    n--;
  if(p->alt && p->base == 16)
    n -= 2;
  else if(p->alt && p->base == 8)
    n--;

  /* Fill with space, before alternate or sign */
  if(!p->lz)
  {
    while(n-- > 0)
      putf(putp, ' ');
  }

  /* print sign */
  if(p->sign)
    putf(putp, p->sign);

  /* Alternate */
  if(p->alt && p->base == 16)
  {
    putf(putp, '0');
    putf(putp, (p->uc ? 'X' : 'x'));
  }
  else if(p->alt && p->base == 8)
  {
    putf(putp, '0');
  }

  /* Fill with zeros, after alternate or sign */
  if(p->lz)
  {
    while(n-- > 0)
      putf(putp, '0');
  }

  /* Put actual buffer */
  bf = p->bf;
  while((ch = *bf++))
    putf(putp, ch);
}

void tfp_format(void *putp, putcf putf, char *fmt, va_list va)
{
  struct param p;
#ifdef PRINTF_LONG_SUPPORT
  char bf[23];
#else
  char bf[12];
#endif
  p.bf = bf;

  char ch;

  while((ch = *(fmt++)))
  {
    if(ch != '%')
    {
      putf(putp, ch);
    }
    else
    {
      /* Init parameter struct */
      p.lz = 0;
      p.alt = 0;
      p.width = 0;
      p.sign = 0;
#ifdef PRINTF_LONG_SUPPORT
      char lng = 0;
#endif

      /* Flags */
      while((ch = *(fmt++)))
      {
        switch(ch)
        {
          case '0':
            p.lz = 1;
            continue;
          case '#':
            p.alt = 1;
            continue;
          default:
            break;
        }
        break;
      }

      /* Width */
      if(ch >= '0' && ch <= '9')
      {
        ch = a2i(ch, &fmt, 10, (int*) &(p.width)); /* d'b: warning fixed */
      }
#ifdef PRINTF_LONG_SUPPORT
      if(ch == 'l')
      {
        ch = *(fmt++);
        lng = 1;
      }
#endif
      switch(ch)
      {
        case 0:
          goto abort;
        case 'u':
          p.base = 10;
#ifdef PRINTF_LONG_SUPPORT
          if(lng)
            uli2a(va_arg(va, unsigned long int), &p);
          else
#endif
            ui2a(va_arg(va, unsigned int), &p);
          putchw(putp, putf, &p);
          break;
        case 'd':
        case 'i':
          p.base = 10;
#ifdef PRINTF_LONG_SUPPORT
          if(lng)
            li2a(va_arg(va, unsigned long int), &p);
          else
#endif
            i2a(va_arg(va, int), &p);
          putchw(putp, putf, &p);
          break;
        case 'x':
        case 'X':
          p.base = 16;
          p.uc = (ch == 'X');
#ifdef PRINTF_LONG_SUPPORT
          if(lng)
            uli2a(va_arg(va, unsigned long int), &p);
          else
#endif
            ui2a(va_arg(va, unsigned int), &p);
          putchw(putp, putf, &p);
          break;
        case 'o':
          p.base = 8;
          ui2a(va_arg(va, unsigned int), &p);
          putchw(putp, putf, &p);
          break;
        case 'c':
          putf(putp, (char) (va_arg(va, int)));
          break;
        case 's':
          p.bf = va_arg(va, char *);
          putchw(putp, putf, &p);
          p.bf = bf;
          break;
        case '%':
          putf(putp, ch);
        default:
          break;
      }
    }
  }
  abort: ;
}

void init_printf(void *putp, void (*putf)(void *, char))
{
  stdout_putf = putf;
  stdout_putp = putp;
}

void tfp_printf(char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  tfp_format(stdout_putp, stdout_putf, fmt, va);
  va_end(va);
}

static void putcp(void *p, char c)
{
  *(*((char **) p))++ = c;
}

void tfp_sprintf(char *s, char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  tfp_format(&s, putcp, fmt, va);
  putcp(&s, 0);
  va_end(va);
}
