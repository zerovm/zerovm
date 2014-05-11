/*
 * Computation of the n'th decimal digit of pi with very little memory.
 * Written by Fabrice Bellard on February 26, 1997.
 * 
 * We use a slightly modified version of the method described by Simon
 * Plouffe in "On the Computation of the n'th decimal digit of various
 * transcendental numbers" (November 1996). We have modified the algorithm
 * to get a running time of O(n^2) instead of O(n^3log(n)^3).
 *
 * This program uses a variation of the formula found by Gosper in 1974 :
 * 
 * pi = sum( (25*n-3)/(binomial(3*n,n)*2^(n-1)), n=0..infinity);
 *
 * This program uses mostly integer arithmetic. It may be slow on some
 * hardwares where integer multiplications and divisons must be done by
 * software. We have supposed that 'int' has a size of at least 32 bits. If
 * your compiler supports 'long long' integers of 64 bits, you may use the
 * integer version of 'mul_mod' (see HAS_LONG_LONG).  
 *
 * ported by d'b
 */

#ifdef __ZEROVM__
#include "include/zvmlib.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define STDERR stderr
#endif

/* uncomment the following line to use 'long long' integers */
/* #define HAS_LONG_LONG */

#ifdef HAS_LONG_LONG
#define mul_mod(a,b,m) (( (long long) (a) * (long long) (b) ) % (m))
#else
#define mul_mod(a,b,m) fmod( (double) a * (double) b, m)
#endif

/* d'b: ### {{ */
int errno = 0;

#if _lib_frexp && _lib_ldexp
NoN(frexp)
#else
#if defined(_ast_dbl_exp_index) && defined(_ast_dbl_exp_shift)

#define INIT()    _ast_dbl_exp_t _pow_
#define pow2(i)    (_pow_.f=1,_pow_.e[_ast_dbl_exp_index]+=((i)<<_ast_dbl_exp_shift),_pow_.f)

#else

#define DBL_MAX_EXP 1024
#ifndef elementsof
#define elementsof(a)  (sizeof(a)/sizeof(a[0]))
#endif

static double pow2tab[DBL_MAX_EXP + 1];

typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned long long uint64_t;
typedef long long int64_t;

union fshape
{
  float value;
  uint32_t bits;
};

union dshape
{
  double value;
  uint64_t bits;
};

/* Get two 32 bit ints from a double.  */
#define EXTRACT_WORDS(hi,lo,d)                                  \
do {                                                            \
  union dshape __u;                                             \
  __u.value = (d);                                              \
  (hi) = __u.bits >> 32;                                        \
  (lo) = (uint32_t)__u.bits;                                    \
} while (0)

/* Set a double from two 32 bit ints.  */
#define INSERT_WORDS(d,hi,lo)                                   \
do {                                                            \
  union dshape __u;                                             \
  __u.bits = ((uint64_t)(hi) << 32) | (uint32_t)(lo);           \
  (d) = __u.value;                                              \
} while (0)

static const double Zero[] =
{0.0, -0.0, };

double fmod(double x, double y)
{
  int32_t n, hx, hy, hz, ix, iy, sx, i;
  uint32_t lx, ly, lz;

  EXTRACT_WORDS(hx, lx, x);
  EXTRACT_WORDS(hy, ly, y);
  sx = hx & 0x80000000; /* sign of x */
  hx ^= sx; /* |x| */
  hy &= 0x7fffffff; /* |y| */

  /* purge off exception values */
  if((hy | ly) == 0 || hx >= 0x7ff00000 || /* y=0,or x not finite */
  (hy | ((ly | -ly) >> 31)) > 0x7ff00000) /* or y is NaN */
    return (x * y) / (x * y);
  if(hx <= hy)
  {
    if(hx < hy || lx < ly) /* |x| < |y| */
      return x;
    if(lx == ly) /* |x| = |y|, return x*0 */
      return Zero[(uint32_t) sx >> 31];
  }

  /* determine ix = ilogb(x) */
  if(hx < 0x00100000)
  { /* subnormal x */
    if(hx == 0)
    {
      for(ix = -1043, i = lx; i > 0; i <<= 1)
        ix -= 1;
    }
    else
    {
      for(ix = -1022, i = hx << 11; i > 0; i <<= 1)
        ix -= 1;
    }
  }
  else
    ix = (hx >> 20) - 1023;

  /* determine iy = ilogb(y) */
  if(hy < 0x00100000)
  { /* subnormal y */
    if(hy == 0)
    {
      for(iy = -1043, i = ly; i > 0; i <<= 1)
        iy -= 1;
    }
    else
    {
      for(iy = -1022, i = hy << 11; i > 0; i <<= 1)
        iy -= 1;
    }
  }
  else
    iy = (hy >> 20) - 1023;

  /* set up {hx,lx}, {hy,ly} and align y to x */
  if(ix >= -1022)
    hx = 0x00100000 | (0x000fffff & hx);
  else
  { /* subnormal x, shift x to normal */
    n = -1022 - ix;
    if(n <= 31)
    {
      hx = (hx << n) | (lx >> (32 - n));
      lx <<= n;
    }
    else
    {
      hx = lx << (n - 32);
      lx = 0;
    }
  }
  if(iy >= -1022)
    hy = 0x00100000 | (0x000fffff & hy);
  else
  { /* subnormal y, shift y to normal */
    n = -1022 - iy;
    if(n <= 31)
    {
      hy = (hy << n) | (ly >> (32 - n));
      ly <<= n;
    }
    else
    {
      hy = ly << (n - 32);
      ly = 0;
    }
  }

  /* fix point fmod */
  n = ix - iy;
  while(n--)
  {
    hz = hx - hy;
    lz = lx - ly;
    if(lx < ly)
      hz -= 1;
    if(hz < 0)
    {
      hx = hx + hx + (lx >> 31);
      lx = lx + lx;
    }
    else
    {
      if((hz | lz) == 0) /* return sign(x)*0 */
        return Zero[(uint32_t) sx >> 31];
      hx = hz + hz + (lz >> 31);
      lx = lz + lz;
    }
  }
  hz = hx - hy;
  lz = lx - ly;
  if(lx < ly)
    hz -= 1;
  if(hz >= 0)
  {
    hx = hz;
    lx = lz;
  }

  /* convert back to floating value and restore the sign */
  if((hx | lx) == 0) /* return sign(x)*0 */
    return Zero[(uint32_t) sx >> 31];
  while(hx < 0x00100000)
  { /* normalize x */
    hx = hx + hx + (lx >> 31);
    lx = lx + lx;
    iy -= 1;
  }
  if(iy >= -1022)
  { /* normalize output */
    hx = ((hx - 0x00100000) | ((iy + 1023) << 20));
    INSERT_WORDS(x, hx|sx, lx);
  }
  else
  { /* subnormal output */
    n = -1022 - iy;
    if(n <= 20)
    {
      lx = (lx >> n) | ((uint32_t) hx << (32 - n));
      hx >>= n;
    }
    else if(n <= 31)
    {
      lx = (hx << (32 - n)) | (lx >> n);
      hx = sx;
    }
    else
    {
      lx = hx >> (n - 32);
      hx = sx;
    }
    INSERT_WORDS(x, hx|sx, lx);
  }
  return x; /* exact output */
}

static const double tiny = 1.0e-300;

double sqrt(double x)
{
  double z;
  int32_t sign = (int) 0x80000000;
  int32_t ix0, s0, q, m, t, i;
  uint32_t r, t1, s1, ix1, q1;

  EXTRACT_WORDS(ix0, ix1, x);

  /* take care of Inf and NaN */
  if((ix0 & 0x7ff00000) == 0x7ff00000)
  {
    return x * x + x; /* sqrt(NaN)=NaN, sqrt(+inf)=+inf, sqrt(-inf)=sNaN */
  }
  /* take care of zero */
  if(ix0 <= 0)
  {
    if(((ix0 & ~sign) | ix1) == 0)
      return x; /* sqrt(+-0) = +-0 */
    if(ix0 < 0)
      return (x - x) / (x - x); /* sqrt(-ve) = sNaN */
  }
  /* normalize x */
  m = ix0 >> 20;
  if(m == 0)
  { /* subnormal x */
    while(ix0 == 0)
    {
      m -= 21;
      ix0 |= (ix1 >> 11);
      ix1 <<= 21;
    }
    for(i = 0; (ix0 & 0x00100000) == 0; i++)
      ix0 <<= 1;
    m -= i - 1;
    ix0 |= ix1 >> (32 - i);
    ix1 <<= i;
  }
  m -= 1023; /* unbias exponent */
  ix0 = (ix0 & 0x000fffff) | 0x00100000;
  if(m & 1)
  { /* odd m, double x to make it even */
    ix0 += ix0 + ((ix1 & sign) >> 31);
    ix1 += ix1;
  }
  m >>= 1; /* m = [m/2] */

  /* generate sqrt(x) bit by bit */
  ix0 += ix0 + ((ix1 & sign) >> 31);
  ix1 += ix1;
  q = q1 = s0 = s1 = 0; /* [q,q1] = sqrt(x) */
  r = 0x00200000; /* r = moving bit from right to left */

  while(r != 0)
  {
    t = s0 + r;
    if(t <= ix0)
    {
      s0 = t + r;
      ix0 -= t;
      q += r;
    }
    ix0 += ix0 + ((ix1 & sign) >> 31);
    ix1 += ix1;
    r >>= 1;
  }

  r = sign;
  while(r != 0)
  {
    t1 = s1 + r;
    t = s0;
    if(t < ix0 || (t == ix0 && t1 <= ix1))
    {
      s1 = t1 + r;
      if((t1 & sign) == sign && (s1 & sign) == 0)
        s0++;
      ix0 -= t;
      if(ix1 < t1)
        ix0--;
      ix1 -= t1;
      q1 += r;
    }
    ix0 += ix0 + ((ix1 & sign) >> 31);
    ix1 += ix1;
    r >>= 1;
  }

  /* use floating add to find out rounding direction */
  if((ix0 | ix1) != 0)
  {
    z = 1.0 - tiny; /* raise inexact flag */
    if(z >= 1.0)
    {
      z = 1.0 + tiny;
      if(q1 == (uint32_t) 0xffffffff)
      {
        q1 = 0;
        q++;
      }
      else if(z > 1.0)
      {
        if(q1 == (uint32_t) 0xfffffffe)
          q++;
        q1 += 2;
      }
      else
        q1 += q1 & 1;
    }
  }
  ix0 = (q >> 1) + 0x3fe00000;
  ix1 = q1 >> 1;
  if(q & 1)
    ix1 |= sign;
  ix0 += m << 20;
  INSERT_WORDS(z, ix0, ix1);
  return z;
}

static int init(void)
{
  register int x;
  double g;

  g = 1;
  for(x = 0; x < elementsof(pow2tab); x++)
  {
    pow2tab[x] = g;
    g *= 2;
  }
  return 0;
}

#define INIT()    (pow2tab[0]?0:init())

#define pow2(i)    pow2tab[i]

#endif

#if !_lib_frexp
extern double frexp(double f, int* p)
{
  register int k;
  register int x;
  double g;

  INIT();

  /*
   * normalize
   */

  x = k = DBL_MAX_EXP / 2;
  if(f < 1)
  {
    g = 1.0L / f;
    for(;;)
    {
      k = (k + 1) / 2;
      if(g < pow2(x))
        x -= k;
      else if(k == 1 && g < pow2(x+1))
        break;
      else
        x += k;
    }
    if(g == pow2(x))
      x--;
    x = -x;
  }
  else if(f > 1)
  {
    for(;;)
    {
      k = (k + 1) / 2;
      if(f > pow2(x))
        x += k;
      else if(k == 1 && f > pow2(x-1))
        break;
      else
        x -= k;
    }
    if(f == pow2(x))
      x++;
  }
  else
    x = 1;
  *p = x;

  /*
   * shift
   */

  x = -x;
  if(x < 0)
    f /= pow2(-x);
  else if(x < DBL_MAX_EXP)
    f *= pow2(x);
  else
    f = (f * pow2(DBL_MAX_EXP - 1)) * pow2(x - (DBL_MAX_EXP - 1));
  return f;
}
#endif

#if !_lib_ldexp
extern double ldexp(double f, register int x)
{
  INIT();
  if(x < 0)
    f /= pow2(-x);
  else if(x < DBL_MAX_EXP)
    f *= pow2(x);
  else
    f = (f * pow2(DBL_MAX_EXP - 1)) * pow2(x - (DBL_MAX_EXP - 1));
  return f;
}
#endif /* !_lib_ldexp */

#endif

int __IsNan(double d)
{
  float f = d;
  long *l = (long*)&f;

  return (*l & 0x7f800000) == 0x7f800000 && (*l & 0x007fffff) != 0 ? 1 : 0;
}

/*
 * (c) copyright 1988 by the Vrije Universiteit, Amsterdam, The Netherlands.
 * See the copyright notice in the ACK home directory, in the file "Copyright".
 *
 * Author: Ceriel J.H. Jacobs
 */
/* $Header: /users/cosc/staff/paul/CVS/minix1.7/src/lib/math/log.c,v 1.2 1996/04/10 21:15:32 paul Exp $ */

#define POLYNOM1(x, a)  ((a)[1]*(x)+(a)[0])
#define POLYNOM2(x, a)  (POLYNOM1((x),(a)+1)*(x)+(a)[0])
#define POLYNOM3(x, a)  (POLYNOM2((x),(a)+1)*(x)+(a)[0])

/* errno codes */
#define EDOM        33  /* Math argument out of domain of func */
#define ERANGE      34  /* Math result not representable */

/* other definitions */
#define HUGE_VAL -1 /* ### */
#define DBL_MAX 1.7976931348623157e+308
#define M_1_SQRT2 0.70710678118654752440084436210484904

double log(double x)
{
  /* Algorithm and coefficients from:
   "Software manual for the elementary functions"
   by W.J. Cody and W. Waite, Prentice-Hall, 1980
   */
  static double a[] =
  {-0.64124943423745581147e2, 0.16383943563021534222e2, -0.78956112887491257267e0};
  static double b[] =
  {-0.76949932108494879777e3, 0.31203222091924532844e3, -0.35667977739034646171e2, 1.0};

  double znum, zden, z, w;
  int exponent;

  if(__IsNan(x))
  {
    errno = EDOM;
    return x;
  }
  if(x < 0)
  {
    errno = EDOM;
    return -HUGE_VAL;
  }
  else if(x == 0)
  {
    errno = ERANGE;
    return -HUGE_VAL;
  }

  if(x <= DBL_MAX)
  {
  }
  else
    return x; /* for infinity and Nan */
  x = frexp(x, &exponent);
  if(x > M_1_SQRT2)
  {
    znum = (x - 0.5) - 0.5;
    zden = x * 0.5 + 0.5;
  }
  else
  {
    znum = x - 0.5;
    zden = znum * 0.5 + 0.5;
    exponent--;
  }
  z = znum / zden;
  w = z * z;
  x = z + z * w * (POLYNOM2(w,a) / POLYNOM3(w,b));
  z = exponent;
  x += z * (-2.121944400546905827679e-4);
  return x + z * 0.693359375;
}
/* }} */

/* return the inverse of x mod y */
int inv_mod(int x, int y)
{
  int q, u, v, a, c, t;

  u = x;
  v = y;
  c = 1;
  a = 0;
  do
  {
    q = v / u;

    t = c;
    c = a - q * c;
    a = t;

    t = u;
    u = v - q * u;
    v = t;
  } while(u != 0);
  a = a % y;
  if(a < 0)
    a = y + a;
  return a;
}

/* return the inverse of u mod v, if v is odd */
int inv_mod2(int u, int v)
{
  int u1, u3, v1, v3, t1, t3;

  u1 = 1;
  u3 = u;

  v1 = v;
  v3 = v;

  if((u & 1) != 0)
  {
    t1 = 0;
    t3 = -v;
    goto Y4;
  }
  else
  {
    t1 = 1;
    t3 = u;
  }

  do
  {

    do
    {
      if((t1 & 1) == 0)
      {
        t1 = t1 >> 1;
        t3 = t3 >> 1;
      }
      else
      {
        t1 = (t1 + v) >> 1;
        t3 = t3 >> 1;
      }
      Y4: ;
    } while((t3 & 1) == 0);

    if(t3 >= 0)
    {
      u1 = t1;
      u3 = t3;
    }
    else
    {
      v1 = v - t1;
      v3 = -t3;
    }
    t1 = u1 - v1;
    t3 = u3 - v3;
    if(t1 < 0)
    {
      t1 = t1 + v;
    }
  } while(t3 != 0);
  return u1;
}

/* return (a^b) mod m */
int pow_mod(int a, int b, int m)
{
  int r, aa;

  r = 1;
  aa = a;
  while(1)
  {
    if(b & 1)
      r = mul_mod(r, aa, m);
    b = b >> 1;
    if(b == 0)
      break;
    aa = mul_mod(aa, aa, m);
  }
  return r;
}

/* return true if n is prime */
int is_prime(int n)
{
  int r, i;
  if((n % 2) == 0)
    return 0;

  r = (int) (sqrt(n));
  for(i = 3; i <= r; i += 2)
    if((n % i) == 0)
      return 0;
  return 1;
}

/* return the prime number immediatly after n */
int next_prime(int n)
{
  do
  {
    n++;
  } while(!is_prime(n));
  return n;
}

#define DIVN(t,a,v,vinc,kq,kqinc) \
{                                 \
  kq+=kqinc;                      \
  if (kq >= a) {                  \
    do { kq-=a; } while (kq>=a);  \
    if (kq == 0) {                \
      do {                        \
  t=t/a;                          \
  v+=vinc;                        \
      } while ((t % a) == 0);     \
    }                             \
  }                               \
}

int main(int argc, char *argv[])
{
  int av, a, vmax, N, n = 0, num, den, k, kq1, kq2, kq3, kq4, t, v, s, i, t1;
  double sum;

  if(argc < 2 || (n = atoi(argv[1])) <= 0)
  {
    printf("This program computes the n'th decimal digit of pi\n"
        "usage: pi n , where n is the digit you want\n");
    exit(1);
  }

  N = (int) ((n + 20) * log(10) / log(13.5));
  sum = 0;

  for(a = 2; a <= (3 * N); a = next_prime(a))
  {
    vmax = (int) (log(3 * N) / log(a));
    if(a == 2)
    {
      vmax = vmax + (N - n);
      if(vmax <= 0)
        continue;
    }
    av = 1;
    for(i = 0; i < vmax; i++)
      av = av * a;

    s = 0;
    den = 1;
    kq1 = 0;
    kq2 = -1;
    kq3 = -3;
    kq4 = -2;
    if(a == 2)
    {
      num = 1;
      v = -n;
    }
    else
    {
      num = pow_mod(2, n, av);
      v = 0;
    }

    for(k = 1; k <= N; k++)
    {
      t = 2 * k;
      DIVN(t, a, v, -1, kq1, 2);
      num = mul_mod(num, t, av);

      t = 2 * k - 1;
      DIVN(t, a, v, -1, kq2, 2);
      num = mul_mod(num, t, av);

      t = 3 * (3 * k - 1);
      DIVN(t, a, v, 1, kq3, 9);
      den = mul_mod(den, t, av);

      t = (3 * k - 2);
      DIVN(t, a, v, 1, kq4, 3);
      if(a != 2)
        t = t * 2;
      else
        v++;
      den = mul_mod(den, t, av);

      if(v > 0)
      {
        if(a != 2)
          t = inv_mod2(den, av);
        else
          t = inv_mod(den, av);
        t = mul_mod(t, num, av);
        for(i = v; i < vmax; i++)
          t = mul_mod(t, a, av);
        t1 = (25 * k - 3);
        t = mul_mod(t, t1, av);
        s += t;
        if(s >= av)
          s -= av;
      }
    }

    t = pow_mod(5, n - 1, av);
    s = mul_mod(s, t, av);
    sum = fmod(sum + (double) s / (double) av, 1.0);
  }
  printf("Decimal digits of pi at position %d: %09d\n", n, (int) (sum * 1e9));
  return 0;
}
