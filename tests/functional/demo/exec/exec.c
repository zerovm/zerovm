/* demonstration of trap jail / unjail functions */
#include "include/zvmlib.h"
#include "include/ztest.h"

#define SIZE 64

int main()
{
  char *p, *g;
  int i;
  UNREFERENCED_VAR(errcount);

  /* allocate buffer */
  printf("allocate buffer\n");
  p = malloc(SIZE + PAGESIZE);
  if(p == NULL)
  {
    fprintf(STDERR, "cannot allocate memory\n");
    return 1;
  }

  /* align buffer */
  g = p;
  p = (char*)(uintptr_t)(ROUNDUP_64K((uintptr_t)p));
  printf("p = 0x%X, aligned = 0x%X\n", g, p);

  /* put code to buffer */
  printf("populate buffer with code\n");
  for(i = 0; i < SIZE; ++i)
    p[i] = 0x90; /* nop */

  /* jail the buffer */
  printf("jail the code\n");
  i = zvm_jail(p, SIZE);
  if(i != 0)
  {
    fprintf(STDERR, "cannot jail the code\n");
    return 2;
  }

  /* unjail the buffer */
  printf("unjail the code\n");
  i = zvm_unjail(p, SIZE);
  if(i != 0)
  {
    fprintf(STDERR, "cannot unjail the code\n");
    return 2;
  }

  /* try to write to buffer */
  printf("write the buffer\n");
  for(i = 0; i < SIZE; ++i)
    p[i] = 0xdb;

  /* write to stdout channel */
  printf("success\n");
  free(g);

  return 0;
}
