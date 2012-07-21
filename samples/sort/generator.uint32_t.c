/*
 * generate 32-bit random unique numbers and store it to given file
 * usage: generator <output file name> <size of data in mb>
 *
 * update: usage changed. all command line switches must be specified
 * via manifest (see manifest keywords)
 *
 * note: program can only write as much bytes as allowed in manifest
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "api/zrt.h"

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
  FILE *f;

  /*
   * unlike previous generator version (resided in "samples/sort")
   * this program use paginated channel instead of mapped
   */

  /* check command line */
  if(argc != 3)
  {
    fprintf(stderr, "usage: generator number_of_elements file_name\n");
    return 1;
  }

  /* open file to output numbers */
  f = fopen(argv[2], "wb");
  if(f == NULL)
  {
    fprintf(stderr, "cannot open output file\n");
    return 2;
  }

  /* get buffer for the output channel */
  seq_size = strtoll(argv[1], NULL, 10);
  inc = -1U / seq_size; /* increment */
  r = malloc(seq_size * sizeof(*r));

  /* check output buffer */
  if(r == NULL || seq_size < 1)
  {
    fprintf(stderr, "OutputChannel is not initialized "
            "addr = 0x%X, size = %d\n",
            (intptr_t) r, seq_size);
    return 3;
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

  /* write data to the output channel */
  i = fwrite(r, sizeof(*r), seq_size, f);
  fprintf(stderr, "%u elements is written\n", i);
  if(i != seq_size)
    fprintf(stderr, "ERROR: couldn't write all generated elements!\n");

  fclose(f);
  return 0;
}
