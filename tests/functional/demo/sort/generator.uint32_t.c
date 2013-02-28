/*
 * generate 32-bit random unique numbers and store it to stdout
 * usage: generator <size of data in mb>
 */

#include <errno.h>
#include "include/zvmlib.h"

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
  uint32_t *buf;
  uint32_t seq_size;
  uint32_t inc;

  /* check the command line */
  if(argc != 2)
  {
    FPRINTF(STDERR, "usage: generator number_of_elements [file_name]\n");
    return EPERM;
  }

  /* get buffer for the output channel */
  seq_size = STRTOL(argv[1], NULL, 10) / sizeof *buf;
  inc = -1U / seq_size; /* increment */
  buf = MALLOC(seq_size * sizeof *buf);

  /* check output buffer */
  if(buf == NULL || seq_size < 1)
  {
    FPRINTF(STDERR, "OutputChannel is not initialized "
        "addr = 0x%X, size = %d\n", (intptr_t) buf, seq_size);
    return ENOMEM;
  }

  /* populate array with sequence of numbers */
  FPRINTF(STDERR, "generating %u of %u-bit numbers..\n", seq_size, 8 * sizeof *buf);
  for(i = 0; i < seq_size; ++i)
    buf[i] = i * inc;

  /* make a mess in the array */
  for(i = 0; i < seq_size; ++i)
  {
    index = ((uint32_t) (RAND() << 22 ^ RAND() << 11 ^ RAND())) % seq_size;
    swap(&buf[i], &buf[index]);
  }
  FPRINTF(STDERR, "numbers generated\n");

  /* write data to the output channel */
  i = WRITE(STDOUT, (const char*)buf, seq_size * sizeof *buf) / sizeof *buf;
  FPRINTF(STDERR, "%u elements is written\n", i);
  if(i != seq_size)
  {
    FPRINTF(STDERR, "ERROR: couldn't write generated elements!\n");
    return EIO;
  }

  FPRINTF(STDERR, "success\n");
  return 0;
}
