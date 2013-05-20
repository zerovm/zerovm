/*
 * sizeof of main types
 * differences between host (x86-64) and guest (nacl_x86-64)
 * type           host guest
 * long           8    4
 * long double    16   8
 * void*          8    4 (any pointer)
 */
#ifdef __ZEROVM__
#include "include/zvmlib.h"
#else
#include <stdio.h>
#endif

int main()
{
  printf("size of char = %d\n", (int)sizeof(char));
  printf("size of unsigned char = %d\n", (int)sizeof(unsigned char));
  printf("size of short = %d\n", (int)sizeof(short));
  printf("size of unsigned short = %d\n", (int)sizeof(unsigned short));
  printf("size of int = %d\n", (int)sizeof(int));
  printf("size of unsigned int = %d\n", (int)sizeof(unsigned int));
  printf("size of long = %d\n", (int)sizeof(long));
  printf("size of unsigned long = %d\n", (int)sizeof(unsigned long));
  printf("size of long long = %d\n", (int)sizeof(long long));
  printf("size of unsigned long long = %d\n", (int)sizeof(unsigned long long));
  printf("size of float = %d\n", (int)sizeof(float));
  printf("size of double = %d\n", (int)sizeof(double));
  printf("size of long double = %d\n", (int)sizeof(long double));
  printf("size of void = %d\n", (int)sizeof(void));
  printf("size of void* = %d\n", (int)sizeof(void*));
  printf("size of enum = %d\n", (int)sizeof(enum {A}));

  return 0;
}
