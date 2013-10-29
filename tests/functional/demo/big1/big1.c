/*
 * the big one
 */
#ifdef __ZEROVM__
#include "include/zvmlib.h"
#else
#include <stdio.h>
#include <stdint.h>
#define STDERR stderr
#endif

int main()
{
  long long i = 0;

#include "big1.h"

  return i;
}
