/*
 * generate 32-bit random unique numbers and store it to given file
 * usage: generator <output file name> <size of data in mb>
 *
 * update: usage changed. all command line switches must be specified
 * via manifest (see manifest keywords)
 *
 * note: program will generate as much numbers as allowed in manifest
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "api/zvm.h"

inline void swap(uint32_t *a, uint32_t *b)
{
  uint32_t temp = *a;
  *a = *b;
  *b = temp;
}

int main(int argc, char **argv)
{
  uint32_t i;
  uint32_t index;
  uint32_t *r;
  uint32_t seq_size;
  uint32_t inc;

  /* get buffer, calculate count of elements */
  r = zvm_channel_addr(OutputChannel);
  seq_size = zvm_channel_size(OutputChannel) / sizeof(*r);
  inc = 4294967295U / seq_size; /* increment */

  /* check output buffer */
  if(r == NULL || seq_size < 1)
  {
    fprintf(stderr, "OutputChannel is not initialized "
            "addr = 0x%X, size = %d\n",
            (intptr_t) r, seq_size);
    return 1;
  }

  /* populate array with sequence of numbers */
  fprintf(stderr, "generating %u of %u-bit numbers..\n", seq_size, sizeof(*r) * 8);
  for(i = 0; i < seq_size; ++i)
    r[i] = i * inc;

  /* make a mess in the array */
  for(i = 0; i < seq_size; ++i)
  {
    index = ((uint32_t) (rand() << 22 ^ rand() << 11 ^ rand())) % seq_size;
    swap(&r[i], &r[index]);
  }
  fprintf(stderr, "numbers are generated\n");

  return 0;
}
