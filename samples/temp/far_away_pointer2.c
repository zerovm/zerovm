#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_ARRAY_SIZE 0x400000000LL
int main()
{
  int64_t size;
  int64_t i = 0;
  char *p = NULL;
  
  /* allocate 16gb of memory */
  for(size = MAX_ARRAY_SIZE; size > 0 && (p = (char*)malloc(size)) == NULL; size >>= 1)
  {
    printf("cannot allocate %lld bytes of memory\n", size);
  }
  
  if(p == NULL)
  {
    printf("cannot allocate %lld bytes of memory\n", size);
    return 1;    
  }
  printf("%lld bytes of memory allocated at [0x%llx]\n", size, (int64_t)p);
  
  /* read 1st bytes */
  printf("show 1st %lld bytes:\n\n", size);
  while(i++ < size)
  {
    printf("byte at [0x%jx] = %02x\n", (int64_t)p, *p++);
  }  
  
  /* free and exit */  
  free(p);  
  return 0;
}
