/*
 * test array with 32-bit elements
 * return 0 if data is sorted, otherwise - 1
 *
 * usage: tester <input file name>
 * update: usage has been changed. all parameters must be passed via manifest
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "api/zvm.h"

#define ELEMENT_SIZE sizeof(uint32_t)

int main(int argc, char **argv)
{
  uint32_t i;
  uint32_t *r;
  uint32_t seq_size;
  uint32_t inc;

  /* check if input is valid */
  if(zvm_channel_addr(InputChannel) == NULL || zvm_channel_size(InputChannel) < 1)
  {
    fprintf(stderr, "invalid input map\n");
    return 2;
  }

  /* assign vars to given input */
  seq_size = zvm_channel_size(InputChannel) / sizeof(*r);
  r = zvm_channel_addr(InputChannel);
  fprintf(stderr, "number of elements = %d\n", seq_size);

  /* test order of the numbers */
  inc = 4294967295U / seq_size; /* increment */
  fprintf(stderr, "test if array of %u %u-bit numbers is sorted:\n", seq_size,
          ELEMENT_SIZE * 8);
  for(i = 0; i < seq_size; ++i)
    if(r[i] != i * inc)
    {
      fprintf(stderr, "given data is NOT SORTED\n");
      fprintf(stderr, "[%u] = %u\n", i, r[i]);
      return 3;
    }

  fprintf(stderr, "given data is SORTED\n");
  return 0;
}
