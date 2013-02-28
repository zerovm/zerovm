/*
 * this sample is a simple demonstration of zerovm.
 * the "hello world" program as usual.
 */
#include "include/zvmlib.h"

#define SIZE 256
#define PATTERN 0xDB

int main(int argc, char **argv)
{
  uint8_t *p = MALLOC(SIZE);
  int i;

  /* heap info */
  PRINTF("heap_ptr = 0x%X, heap_size = %d\n",
      (uintptr_t)MANIFEST->heap_ptr, MANIFEST->heap_size);

  /* memory allocated? */
  if(p == NULL)
  {
    PRINTF("memory allocation error\n");
  }
  else
  {
    PRINTF("memory allocated at 0x%X\n", (uintptr_t)p);
  }

  /* set allocated buffer to pattern */
  MEMSET(p, PATTERN, SIZE);
  PRINTF("memory set to 0x%X pattern\n", PATTERN);

  /* check pattern */
  for(i = 0; i < SIZE; ++i)
  {
    if(p[i] != PATTERN)
      PRINTF("invalid pattern 0x%X at 0x%X\n", p[i], (uintptr_t)p + i);
  }
  PRINTF("pattern tested\n");

  FREE(p);

  return 0;
}
