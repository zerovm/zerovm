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
#include "api/zrt.h"

#define ELEMENT_SIZE sizeof(uint32_t)

int64_t file_size(char *name)
{
  FILE *f;

  f = fopen(name, "rb");
  if(f == NULL) return -1; /* file open error */
  if(fseek(f, 0, SEEK_END) != 0) return -1; /* seek error */
  return ftell(f);
}

int main(int argc, char **argv)
{
  uint32_t i;
  uint32_t *r;
  uint32_t seq_size;
  int32_t filesize;
  uint32_t inc;
  FILE *f;

  /* check command line */
  if(argc != 2)
  {
    fprintf(stderr, "usage: tester file_name\n");
    return 1;
  }

  /* open data file and get the size */
  f = fopen(argv[1], "rb");
  if(f == NULL)
  {
    fprintf(stderr, "cannot open input file\n");
    return 2;
  }

  filesize = file_size(argv[1]);
  if(filesize < 0)
  {
    fprintf(stderr, "cannot get data size from the input file\n");
    return 3;
  }

  /* allocate data buffer */
  seq_size = filesize / sizeof(*r);
  r = malloc(filesize);
  if(r == NULL)
  {
    fprintf(stderr, "cannot allocate data buffer\n");
    return 4;
  }
  fprintf(stderr, "number of elements = %d\n", seq_size);

  /* read the data */
  if(fread(r, sizeof(*r), seq_size, stdin) != seq_size)
  {
    fprintf(stderr, "cannot read data from the input file\n");
    return 5;
  }

  /* test order of the numbers */
  inc = 4294967295U / seq_size; /* increment */
  fprintf(stderr, "testing array of %u %u-bit numbers..\n", seq_size, sizeof(*r) * 8);
  for(i = 0; i < seq_size; ++i)
    if(r[i] != i * inc)
    {
      fprintf(stderr, "given data is NOT SORTED\n");
      fprintf(stderr, "[%u] = %u\n", i, r[i]);
      return 4;
    }

  fprintf(stderr, "given data is SORTED\n");
  return 0;
}
