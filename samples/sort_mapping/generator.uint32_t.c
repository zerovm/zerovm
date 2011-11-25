/*
 * generate 32-bit random unique numbers
 * and store it to given file name
 *
 * usage: generator <output file name> <size of data in mb>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "zerovm_manifest.h"

inline void swap(uint32_t *a, uint32_t *b)
{
	uint32_t temp = *a;
  *a = *b;
  *b = temp;
}

int main(int argc, char **argv)
{
	uint32_t i;
	uint32_t index = 0;
	uint32_t *r;
	int seq_size;
	struct MinorManifest *m;
	
	/* check if in argv[0] we got manifest structure */
  m = (struct MinorManifest *)argv[0];
  if(m->mask)
  {
    printf("manifest structure wasn't passed; argv[0] = %s\n", argv[0]);
    return 1;
  }
  
  /* check if output is valid */
  if(m->output_map_file.p == 0 || m->output_map_file.size == 0)
  {
    printf("invalid output map\n");
    return 3;
  }

  /* set output array */
  r = (uint32_t *)m->output_map_file.p; /* pointer to array */
  seq_size = m->output_map_file.size / sizeof(*r); /* count of elements */
  
  /* populate array with sequence of numbers */  
 	printf("generating %u of %lu-bit numbers..\n", seq_size, sizeof(*r)*8);
  for (i = 0; i < seq_size; ++i) r[i] = i;

  /* make a mess in the array */
  for (i = 0; i < seq_size; ++i)
  {
  	index = ((uint32_t)(rand()<<22 ^ rand()<<11 ^ rand()))%seq_size;
  	swap(&r[i], &r[index]);
  }
  printf("\rnumbers are generated\n");
  
  return 0;
}
