/*
 * generate pseudorandom numbers and write it to /dev/stdout
 * note: will generate until timeout or channel limit reached
 */
#include "include/zvmlib.h"

/* A version of Whittle's implementation */
static inline int random(void)
{
  static int seed = 1; /* must not be 0 */
  unsigned hi = 16807 * (seed >> 16);
  unsigned lo = 16807 * (seed & 0xFFFF);

  lo += (hi >> 15) + ((hi & 0x7FFF) << 16);
  return seed = lo > 2147483647 ? lo - 2147483647 : lo;
}

/* populate "buffer" with "size" of random bytes */
static inline void set_random(char *buffer, int size)
{
  while(size)
    buffer[--size] = (char)random();
}

void _start()
{
  char buffer[BIG_ENOUGH];
  int result = 0;

  do {
    set_random(buffer, sizeof buffer);
    result = zvm_pwrite(1, buffer, sizeof buffer, 0);
  } while(result > 0);

  zvm_exit(result);
}
