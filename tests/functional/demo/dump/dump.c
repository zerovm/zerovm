/*
 * dumps whole available memory. from the trampoline start to the stack end
 */
#include "include/api_tools.h"

#define TRAMPOLINE_START (uintptr_t)0x10000
#define SIZEOF_1ST_PART (uintptr_t)zvm_bulk->heap_ptr + zvm_bulk->heap_size
#define STACK_START (uintptr_t)0xFF000000
#define STACK_SIZE (uintptr_t)0x1000000

int main()
{
  zvm_bulk = zvm_init();

  UNREFERENCED_VAR(errcount);

  /* dump memory from the trampoline start to the heap end */
  zwrite(STDOUT, TRAMPOLINE_START, SIZEOF_1ST_PART);

  /* dump stack */
  zwrite(STDOUT, STACK_START, STACK_SIZE);

  return 0;
}
