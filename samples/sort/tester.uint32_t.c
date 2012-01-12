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

#define LOGFIX /* temporary fix until zrt library will be finished */

#ifdef LOGFIX
#define printf(...)\
do {\
  char msg[4096];\
  sprintf(msg, __VA_ARGS__);\
  log_msg(msg);\
} while (0)
#endif

#define ELEMENT_SIZE sizeof(uint32_t)

int main(int argc, char **argv)
{
	uint32_t i;
	uint32_t *r;
	uint32_t seq_size;
	uint32_t inc;
  struct SetupList setup;
  int retcode = ERR_CODE;

  /* setup */
  retcode = zvm_setup(&setup);
  if(retcode) return retcode;

#ifdef LOGFIX
  /* set up the log */
  retcode = log_set(&setup);
  if(retcode) return retcode;
#endif
  
  /* check if input is valid */
  if(!setup.channels[InputChannel].buffer || !setup.channels[InputChannel].bsize)
  {
    printf("invalid input map\n");
    return 2;
  }

  /* assign vars to given input */
	seq_size = setup.channels[InputChannel].bsize / sizeof(*r);
	r = (uint32_t*)setup.channels[InputChannel].buffer;
	printf("number of elements = %d\n", seq_size);

  /* test order of the numbers */
	inc = 4294967295U / seq_size; /* increment */
	printf("test if array of %u %u-bit numbers is sorted:\n", seq_size, ELEMENT_SIZE*8);
  for (i = 0; i < seq_size; ++i)
  	if(r[i] != i * inc)
  	{
  		printf("given data is NOT SORTED\n");
  		printf("[%u] = %u\n", i, r[i]);
  		return 3;
  	}

	printf("given data is SORTED\n");
	return 0;
}
