/*
 * this sample is a simple demonstration of zerovm.
 * the "hello world" program as usual.
 */
#ifdef __ZEROVM__
#include "include/zvmlib.h"
#else
#include <stdio.h>
#define STDERR stderr
#endif

#if 0
inline int handle(const char *alias)
{
  if(alias != NULL && *alias != '\0')
  {
    int i;
    for(i = 0; i < MANIFEST->channels_count; ++i)
      if(STRCMP(MANIFEST->channels[i].name, alias) == 0)
        return i;
  }

  return -1;
}
#endif

int main()
{
  char buf[BIG_ENOUGH];
  int i;

  /* heap ptr */
  tfp_sprintf(buf, "MANIFEST->heap_ptr = 0x%x\n", MANIFEST->heap_ptr);
  zvm_pwrite(1, buf, STRLEN(buf), 0);

  /* heap size */
  tfp_sprintf(buf, "MANIFEST->heap_size = 0x%x\n", MANIFEST->heap_size);
  zvm_pwrite(1, buf, STRLEN(buf), 0);

  /* stack_size */
  tfp_sprintf(buf, "MANIFEST->stack_size = 0x%x\n", MANIFEST->stack_size);
  zvm_pwrite(1, buf, STRLEN(buf), 0);

  /* channels statistics */
  tfp_sprintf(buf, "\nstatistics for %d channels :\n", MANIFEST->channels_count);
  zvm_pwrite(1, buf, STRLEN(buf), 0);
  
  /* channel 0 stats */
  for(i = 0; i < MANIFEST->channels_count; ++i)
  {
    tfp_sprintf(buf, "MANIFEST->channels[%d].name = 0x%x\n", i, MANIFEST->channels[i].name);
    zvm_pwrite(1, buf, STRLEN(buf), 0);

    tfp_sprintf(buf, "MANIFEST->channels[%d].name = %s\n", i, MANIFEST->channels[i].name);
    zvm_pwrite(1, buf, STRLEN(buf), 0);
    zvm_pwrite(1, MANIFEST->channels[i].name, 0xb, 0);
    
    tfp_sprintf(buf, "MANIFEST->channels[%d].type = %d\n", i, MANIFEST->channels[i].type);
    zvm_pwrite(1, buf, STRLEN(buf), 0);
    tfp_sprintf(buf, "MANIFEST->channels[%d].size = %d\n", i, MANIFEST->channels[i].size);
    zvm_pwrite(1, buf, STRLEN(buf), 0);
    
    tfp_sprintf(buf, "MANIFEST->channels[%d].limits[0] = %d\n", i, MANIFEST->channels[i].limits[0]);
    zvm_pwrite(1, buf, STRLEN(buf), 0);
    tfp_sprintf(buf, "MANIFEST->channels[%d].limits[0] = %d\n", i, MANIFEST->channels[i].limits[1]);
    zvm_pwrite(1, buf, STRLEN(buf), 0);
    tfp_sprintf(buf, "MANIFEST->channels[%d].limits[0] = %d\n", i, MANIFEST->channels[i].limits[2]);
    zvm_pwrite(1, buf, STRLEN(buf), 0);
    tfp_sprintf(buf, "MANIFEST->channels[%d].limits[0] = %d\n\n", i, MANIFEST->channels[i].limits[3]);
    zvm_pwrite(1, buf, STRLEN(buf), 0);
  }


  return 0;
}








