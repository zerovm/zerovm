#include <stdio.h>
#include <stdlib.h>

#define ONE_GIG 0x40000000

int main()
{
  int i;

  /* 1st allocation */
  char *p = (char*)malloc(ONE_GIG/2);
  printf("got address = 0x%x\n", (unsigned)p);

  /* test if we realy have allocated the memory */
  printf("memory initialization..\n");
  for(i = 0; i < ONE_GIG/2; ++i)
  {
    p[i] = i;
  }
  printf("memory has been initialized\n");

  free(p);
  return 0;
}
