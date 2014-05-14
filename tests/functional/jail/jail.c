/*
 * functional test of trap function zvm_mprotect()
 */
#include <sys/mman.h>
#include "include/zvmlib.h"
#include "include/ztest.h"

#define NULL_SIZE PAGESIZE
#define TRAMPOLINE_SIZE PAGESIZE
#define NOP 0x90

int main()
{
  uintptr_t heap_end = (uintptr_t)MANIFEST->heap_ptr + MANIFEST->heap_size;
  uintptr_t stack_end = 0x100000000ULL - MANIFEST->stack_size;
  uint8_t *code = (void*)(uintptr_t)ROUNDUP_64K((uintptr_t)MANIFEST->heap_ptr);

  /* is uboot region in heap already (not read only)? */
  ZTEST(zvm_pread(0, heap_end - PAGESIZE, PAGESIZE, 0) >= 0);

  /* try to change NULL protection (locked region) */
  ZTEST(zvm_mprotect(NULL, NULL_SIZE, PROT_NONE) < 0);
  ZTEST(zvm_mprotect(NULL, NULL_SIZE, PROT_READ) < 0);
  ZTEST(zvm_mprotect(NULL, NULL_SIZE, PROT_READ | PROT_WRITE) < 0);
  ZTEST(zvm_mprotect(NULL, NULL_SIZE, PROT_WRITE) < 0);
  ZTEST(zvm_mprotect(NULL, NULL_SIZE, PROT_READ | PROT_EXEC) < 0);

  /* try to change TRAMPOLINE protection (locked region) */
  ZTEST(zvm_mprotect(0x10000, TRAMPOLINE_SIZE, PROT_NONE) < 0);
  ZTEST(zvm_mprotect(0x10000, TRAMPOLINE_SIZE, PROT_READ) < 0);
  ZTEST(zvm_mprotect(0x10000, TRAMPOLINE_SIZE, PROT_READ | PROT_WRITE) < 0);
  ZTEST(zvm_mprotect(0x10000, TRAMPOLINE_SIZE, PROT_WRITE) < 0);
  ZTEST(zvm_mprotect(0x10000, TRAMPOLINE_SIZE, PROT_READ | PROT_EXEC) < 0);

  /* try to change HOLE protection (locked region) */
  ZTEST(heap_end == ROUNDUP_64K(heap_end));
  ZTEST(zvm_mprotect(heap_end, 0x10000, PROT_NONE) < 0);
  ZTEST(zvm_mprotect(heap_end, 0x10000, PROT_READ) < 0);
  ZTEST(zvm_mprotect(heap_end, 0x10000, PROT_READ | PROT_WRITE) < 0);
  ZTEST(zvm_mprotect(heap_end, 0x10000, PROT_WRITE) < 0);
  ZTEST(zvm_mprotect(heap_end, 0x10000, PROT_READ | PROT_EXEC) < 0);

  /* try to change STACK protection (locked region) */
  ZTEST(stack_end == ROUNDUP_64K(stack_end));
  ZTEST(zvm_mprotect(stack_end, 0x10000, PROT_NONE) < 0);
  ZTEST(zvm_mprotect(stack_end, 0x10000, PROT_READ) < 0);
  ZTEST(zvm_mprotect(stack_end, 0x10000, PROT_READ | PROT_WRITE) < 0);
  ZTEST(zvm_mprotect(stack_end, 0x10000, PROT_WRITE) < 0);
  ZTEST(zvm_mprotect(stack_end, 0x10000, PROT_READ | PROT_EXEC) < 0);

  /* prepare and test good code pattern. then return used page to heap */
  memset(code, NOP, PAGESIZE);
  ZTEST(zvm_mprotect(code, PAGESIZE, PROT_READ | PROT_EXEC) == 0);
  ZTEST(zvm_mprotect(code, PAGESIZE, PROT_READ | PROT_WRITE) == 0);

  /* prepare and test bad code pattern (RDTSC). then return used page to heap */
  memset(code, NOP, PAGESIZE);
  code[0] = 0xf;
  code[1] = 0x31;
  ZTEST(zvm_mprotect(code, PAGESIZE, PROT_READ | PROT_EXEC) < 0);
  ZTEST(zvm_mprotect(code, PAGESIZE, PROT_READ | PROT_WRITE) == 0);

  /* prepare and test bad code pattern (incomplete mov rax, ?). then return used page to heap */
  memset(code, NOP, PAGESIZE);
  code[PAGESIZE - 2] = 0x48;
  code[PAGESIZE - 1] = 0xb8;
  ZTEST(zvm_mprotect(code, PAGESIZE, PROT_READ | PROT_EXEC) < 0);
  ZTEST(zvm_mprotect(code, PAGESIZE, PROT_READ | PROT_WRITE) == 0);

  ZREPORT;
  return 0; /* prevent warning */
}
