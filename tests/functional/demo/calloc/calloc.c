/*
 * calloc test (it is not hypervisor test, but zvmlib)
 */
#include "include/zvmlib.h"

#define SIZE 256*1024*1024 /* 256mb */
#define PATTERN 0xDB
#define FAILIF(cond, msg) if(cond) { printf("%s\n", msg); return 1; }

int main(int argc, char **argv)
{
  int i;
  uint8_t *p = calloc(SIZE, sizeof *p);

  /* memory allocated? */
  FAILIF(p == NULL, "memory allocation error");

  /* heap info */
  printf("memory allocated at 0x%X\n", (uintptr_t)p);
  printf("heap_ptr = 0x%X, heap_size = %d\n",
      (uintptr_t)MANIFEST->heap_ptr, MANIFEST->heap_size);

  /* set allocated buffer to pattern */
  memset(p, PATTERN, SIZE);
  printf("memory set to 0x%X pattern\n", PATTERN);

  /* free and calloc again */
  free(p);
  p = calloc(SIZE, sizeof *p);

  /* check if buffer is populated with 0s again */
  for(i = 0; i < SIZE; ++i)
    FAILIF(p[i] != 0, "test failed. invalid pattern found");

  printf("pattern tested successfully\n");
  free(p);
  return 0;
}
