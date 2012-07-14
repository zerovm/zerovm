/*
 * demonstration of the simple zrt memory management.
 * it involves zrt_mmap(), zrt_munmap(), zrt_sysbrk()
 *
 * MaxMem field must be set (in manifest to sane value)
 * this example also uses zerovm api in order to get max_mem
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "api/zrt.h"
#include "api/zvm.h"

#define SMALL_AMOUNT 0x1000000
#define GAP_AMOUNT 0x2000000
#define AVAILABLE_AMOUNT (zvm_mem_size() - (GAP_AMOUNT))
#define VAR_NAME_TO_STRING(var) #var

#define TEST_PTR(ptr)\
if(ptr == NULL)\
{\
  fprintf(stdout, "[%s] memory allocation error\n", VAR_NAME_TO_STRING(ptr));\
}\
else\
{\
  fprintf(stdout, "[%s] was allocated at 0x%X\n", VAR_NAME_TO_STRING(ptr), (uintptr_t)ptr);\
}

int main()
{
  /* first off check manifest */
  fprintf(stdout, "we are using %sTRUSTED memory manager\n",
          zvm_mem_size() ? "UN" : "");

  TEST_PTR(zvm_heap_ptr());

  fprintf(stdout, "initial memory allocation\n");
  /*
   * "auto" variables will be placed to stack. we use them
   * for sake of completeness
   */
  uint32_t auto_var[0x10000];
  TEST_PTR(auto_var);

  /* static variable (for sake of completeness) */

  /* allocation of the small amount of memory */
  printf("mark1\n");
  uint32_t *small_ptr = malloc(SMALL_AMOUNT);
  printf("mark2\n");
  TEST_PTR(small_ptr);

  /* allocation of the huge amount of memory */
  uint32_t *huge_ptr = malloc(AVAILABLE_AMOUNT - GAP_AMOUNT - SMALL_AMOUNT);
  TEST_PTR(huge_ptr);

  /* reallocate memory */
  fprintf(stdout, "memory reallocation\n");
  huge_ptr = realloc(huge_ptr, SMALL_AMOUNT);
  TEST_PTR(huge_ptr);
  small_ptr = realloc(small_ptr, SMALL_AMOUNT);
  TEST_PTR(small_ptr);

  /* free allocated memory */
  fprintf(stdout, "memory deallocation\n");
  free(small_ptr);
  TEST_PTR(small_ptr);
  free(huge_ptr);
  TEST_PTR(huge_ptr);

  /* allocate it again */
  fprintf(stdout, "final memory allocation\n");
  small_ptr = malloc(SMALL_AMOUNT);
  huge_ptr = malloc(AVAILABLE_AMOUNT - GAP_AMOUNT - SMALL_AMOUNT);

  /* free allocated memory */
  fprintf(stdout, "final memory deallocation\n");
  free(small_ptr);
  TEST_PTR(small_ptr);
  free(huge_ptr);
  TEST_PTR(huge_ptr);

  fprintf(stdout, "exiting..\n");
  return 0;
}
