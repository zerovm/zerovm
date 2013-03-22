/*
 * functional test of trap functions jail / unjail
 */
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

  /* validate it */
  result = zvm_jail(p, SIZE);

  /* run it */
  if(result == 0) ((void(*)())p)();

  /*
   * release resources and check if they really available
   * warning: "for" can produce signal #11
   */
  ZFAIL(zvm_unjail(p, SIZE) == 0);
  for(i = 0; i < SIZE; ++i)
    p[i] = 0xdb;

  free(g);
  return result;
}

int main()
{
  ZTEST(test_function(good) == 0);
  ZTEST(test_function((void (*)())bad) != 0);

  ZREPORT;
  return 0; /* prevent warning */
}
