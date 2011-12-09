/*
 * generate 32-bit random unique numbers
 * and store it to given file name
 *
 * usage: generator <output file name> <size of data in mb>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define ELEMENT_SIZE sizeof(uint32_t)

void show_help_and_exit()
{
	printf("usage: permutation <output file name> <size of data in mb>\n");
	exit(1);
}

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
	FILE *out;
	int seq_size = 1024*1024/ELEMENT_SIZE;

  /* check the command line */
 	if (argc < 2 || argc > 3)
		show_help_and_exit();

	if (argc == 3)
	{
		seq_size *= atoi(argv[2]);
		if(seq_size < 1)
			show_help_and_exit();
	}
	printf("generating %u of %u-bit numbers..\n", seq_size, ELEMENT_SIZE*8);

  /* allocate memory */
  printf("memory allocation.. ");
	r = (uint32_t*)malloc(seq_size * ELEMENT_SIZE);
	if(r == NULL)
	{
		printf("\rmemory allocation error\n");
		return 1;
	}
	printf("\rmemory allocated at [0x%X]\n", r);

  /* open output file */
	out = fopen(argv[1], "wb");
	if(out == NULL)
	{
		printf("\routput file open error\n");
		return 1;
	}

  /* populate array with sequence of numbers */
  printf("numbers generation.. ");
  for (i = 0; i < seq_size; ++i) r[i] = i;

  /* make a mess in the array */
  for (i = 0; i < seq_size; ++i)
  {
  	index = ((uint32_t)(rand()<<22 ^ rand()<<11 ^ rand()))%seq_size;
  	swap(&r[i], &r[index]);
  }
  printf("\rnumbers are generated\n");

  /* store results */
  printf("saving results.. ");
  fwrite(r, sizeof(uint32_t), seq_size, out);
  printf("\rresults are stored\n");

  free(r);
	fclose(out);
}