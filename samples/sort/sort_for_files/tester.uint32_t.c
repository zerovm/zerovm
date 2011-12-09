/*
 * test array with 32-bit elements
 * return 0 if data is sorted, otherwise - 1
 *
 * usage: tester <input file name>
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define ELEMENT_SIZE sizeof(uint32_t)

void show_help_and_exit()
{
	printf("usage: tester <input file name>\n");
	exit(1);
}

/* get number of elements in the given file */
int get_elements_count(FILE *in)
{
	int result = 0;

  /* file open error */
	if(in == NULL) return -1;

	fseek(in, 0, SEEK_END);
	result = ftell(in);
	fseek(in, 0, SEEK_SET);

  /* file does not contain 32-bit integers */
	if(result & (ELEMENT_SIZE - 1)) return -1;

	return result/ELEMENT_SIZE;
}

int main(int argc, char **argv)
{
	uint32_t i;
	uint32_t *r;
	FILE *in = NULL;
	int seq_size = 1024*1024/ELEMENT_SIZE;

  /* check the command line */
 	if (argc != 2)
		show_help_and_exit();

  /* open input file */
  printf("open data to test.. ");
	in = fopen(argv[1], "rb");
	if(in == NULL)
	{
		printf("\rinput file open error\n");
		return 1;
	}

	seq_size = get_elements_count(in);
	if(seq_size < 0)
	{
		printf("cannot get number of elements from the input file\n");
		return 1;
	}
	printf("number of elements = %d\n", seq_size);

  /* allocate memory */
  printf("memory allocation.. ");
	r = (uint32_t*)malloc(seq_size * ELEMENT_SIZE);
	if(r == NULL)
	{
		printf("\rmemory allocation error\n");
		return 1;
	}
	printf("\rmemory allocated at [0x%X]\n", r);

  /* read sorted data */
  fread(r, sizeof(uint32_t), seq_size, in);
  printf("\rdata to test been read\n");

	printf("test if array of %u %u-bit numbers is sorted:\n", seq_size, ELEMENT_SIZE*8);

  /* test order of the numbers */
  printf("test.. ");
  for (i = 0; i < seq_size; ++i)
  	if(r[i] != i)
  	{
  		printf("given data is NOT SORTED\n");
  		printf("[%u] = %u\n", i, r[i]);
  		return 1;
  	}

  free(r);
	fclose(in);

	printf("given data is SORTED\n");
	return 0;
}