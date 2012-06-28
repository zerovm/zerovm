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
#include <sys/stat.h>
#include "api/zvm.h"

#define ELEMENT_SIZE sizeof(uint32_t)

int main(int argc, char **argv)
{
	uint32_t i;
	uint32_t *r;
	uint32_t seq_size;
	uint32_t inc;
  struct stat fs;

  /* get input channel size */
  if(fstat(fileno(stdin), &fs) == -1)
  {
    fprintf(stderr, "cannot get data size from the input channel\n");
    return 1;
  }

  /* allocate data buffer */
  seq_size = fs.st_size / sizeof(*r);
  r = malloc(fs.st_size);
  if(r == NULL)
  {
    fprintf(stderr, "cannot allocate data buffer\n");
    return 2;
  }
  fprintf(stderr, "number of elements = %d\n", seq_size);

  /* read the data */
  if(fread(r, sizeof(*r), seq_size, stdin) != seq_size)
  {
    fprintf(stderr, "cannot read data from the input channel\n");
    return 3;
  }

  /* test order of the numbers */
	inc = 4294967295U / seq_size; /* increment */
	fprintf(stderr, "testing array of %u %u-bit numbers..\n", seq_size, sizeof(*r)*8);
  for (i = 0; i < seq_size; ++i)
  	if(r[i] != i * inc)
  	{
  		fprintf(stderr, "given data is NOT SORTED\n");
  		fprintf(stderr, "[%u] = %u\n", i, r[i]);
  		return 4;
  	}

	fprintf(stderr, "given data is SORTED\n");
	return 0;
}
