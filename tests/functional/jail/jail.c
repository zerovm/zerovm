/*
 * functional test of trap functions jail / unjail
 */
#include <sys/mman.h> /* PROT_READ, PROT_EXEC, PROT_WRITE */
#include "include/zvmlib.h"
#include "include/ztest.h"

/*
 * should be large enough to hold one small function and
 * small enough to be less or equal than text segment size
 */
#define SIZE 0x10000

/* should pass validation */
static void good()
{
  ZTEST("good function");
}

/* should not pass validation */
static char bad[] = {0xf, 0x31, 0xc3};

static int test_function(void(*func)())
{
  char *p, *g;
  int i, result;

  /* allocate and align buffer */
  p = malloc(SIZE + PAGESIZE);
  ZFAIL(p != NULL);
  g = p;
  p = (char*)(uintptr_t)(ROUNDUP_64K((uintptr_t)p));

  /* store a good function to buffer */
  memcpy(p, func, SIZE);

  /* validate it (put r/x protection) */
  result = zvm_mprotect(p, SIZE, PROT_READ | PROT_EXEC);

  /* run it */
  if(result == 0) ((void(*)())p)();

  /*
   * put r/w protection (release memory region from r/x)
   * warning: "for" can produce signal #11
   */
  result |= zvm_mprotect(p, SIZE, PROT_READ | PROT_WRITE);
  for(i = 0; i < SIZE; ++i)
    p[i] = 0xdb;

  free(g);
  return result;
}

int main()
{
  ZTEST(test_function(good) == 0);
  ZTEST(test_function((void (*)())bad) != 0);
  
  /* try to change NULL protection */
  ZTEST(zvm_mprotect(NULL, 0x10000, PROT_NONE) == -13);
  ZTEST(zvm_mprotect(NULL, 0x10000, PROT_READ) == -13);
  ZTEST(zvm_mprotect(NULL, 0x10000, PROT_READ | PROT_WRITE) == -13);
  ZTEST(zvm_mprotect(NULL, 0x10000, PROT_WRITE) == -13);
  ZTEST(zvm_mprotect(NULL, 0x10000, PROT_READ | PROT_EXEC) == -13);

  /* try to change TRAMPOLINE protection */
  ZTEST(zvm_mprotect(0x10000, 0x10000, PROT_NONE) == -13);
  ZTEST(zvm_mprotect(0x10000, 0x10000, PROT_READ) == -13);
  ZTEST(zvm_mprotect(0x10000, 0x10000, PROT_READ | PROT_WRITE) == -13);
  ZTEST(zvm_mprotect(0x10000, 0x10000, PROT_WRITE) == -13);
  ZTEST(zvm_mprotect(0x10000, 0x10000, PROT_READ | PROT_EXEC) == -13);

  /* try to change MANIFEST protection */
  ZTEST(zvm_mprotect(0xFEFF0000, 0x10000, PROT_NONE) == -13);
  ZTEST(zvm_mprotect(0xFEFF0000, 0x10000, PROT_READ) == -13);
  ZTEST(zvm_mprotect(0xFEFF0000, 0x10000, PROT_READ | PROT_WRITE) == -13);
  ZTEST(zvm_mprotect(0xFEFF0000, 0x10000, PROT_WRITE) == -13);
  ZTEST(zvm_mprotect(0xFEFF0000, 0x10000, PROT_READ | PROT_EXEC) == -13);

  /* try to change STACK protection */
  ZTEST(zvm_mprotect(0xFF000000, 0x10000, PROT_NONE) == -13);
  ZTEST(zvm_mprotect(0xFF000000, 0x10000, PROT_READ) == -13);
  ZTEST(zvm_mprotect(0xFF000000, 0x10000, PROT_READ | PROT_WRITE) == -13);
  ZTEST(zvm_mprotect(0xFF000000, 0x10000, PROT_WRITE) == -13);
  ZTEST(zvm_mprotect(0xFF000000, 0x10000, PROT_READ | PROT_EXEC) == -13);

  ZREPORT;
  return 0; /* prevent warning */
}
