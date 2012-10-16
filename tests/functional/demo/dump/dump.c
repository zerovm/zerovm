/*
 * dumps whole available memory. from the trampoline start to
 * the stack end. creates the memory map.
 */
#include "include/api_tools.h"

#define TRAMPOLINE_START (uintptr_t)0x10000
#define HEAP_END ((uintptr_t)zvm_bulk->heap_ptr + zvm_bulk->heap_size)
#define STACK_END (uintptr_t)(-1U + 1 - STACK_SIZE)

int main()
{
  zvm_bulk = zvm_init();

  UNREFERENCED_VAR(errcount);

  /* memory map */
  ZPRINTF(STDERR, "%-16s = 0x%09X\n", "trampoline start", TRAMPOLINE_START);
  ZPRINTF(STDERR, "%-16s = 0x%09X\n", "heap start", (uintptr_t)zvm_bulk->heap_ptr);
  ZPRINTF(STDERR, "%-16s = 0x%09X\n", "heap end", HEAP_END);
  ZPRINTF(STDERR, "%-16s = 0x%09X\n", "stack end", STACK_END);
  ZPRINTF(STDERR, "%-16s = 0x%09llX\n", "stack start", STACK_END + (uint64_t)STACK_SIZE);

  /* dump memory from the trampoline start to the heap end */
  zwrite(STDOUT, (void*)TRAMPOLINE_START, HEAP_END - TRAMPOLINE_START);

  /* dump stack */
  zwrite(STDOUT, (void*)STACK_END, STACK_SIZE);

  return 0;
}
