/*
 * test array with 32-bit elements
 * return 0 if data is sorted, otherwise - 1
 *
 * usage: tester <input file name>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "zerovm_manifest.h"

#define ELEMENT_SIZE sizeof(uint32_t)

int main(int argc, char **argv)
{
	uint32_t i;
	uint32_t *r;
	uint32_t seq_size;
	struct MinorManifest *m;

	/* check if in argv[0] we got manifest structure */
  m = (struct MinorManifest *)argv[0];
  if(m->mask)
  {
    printf("manifest structure wasn't passed; argv[0] = %s\n", argv[0]);
    return 1;
  }  
  
  /* check if input is valid */
  if(m->input_map_file.p == 0 || m->input_map_file.size == 0)
  {
    printf("invalid input map\n");
    return 2;
  }

  /* assign vars to given input */
	seq_size = m->input_map_file.size / sizeof(*r);
	r = (uint32_t*)m->input_map_file.p;
	printf("number of elements = %d\n", seq_size);

  /* test order of the numbers */
	printf("test if array of %u %u-bit numbers is sorted:\n", seq_size, ELEMENT_SIZE*8);
  for (i = 0; i < seq_size; ++i)
  	if(r[i] != i)
  	{
  		printf("given data is NOT SORTED\n");
  		printf("[%u] = %u\n", i, r[i]);
  		return 3;
  	}

	printf("given data is SORTED\n");
	return 0;
}
