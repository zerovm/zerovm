/*
 * dumps all user manifest fields
 */
#include "include/zvmlib.h"

#define DUMP(fmt, a) \
  do { \
    char buf[BIG_ENOUGH]; \
    tfp_sprintf(buf, "%s = " fmt "\n", #a, a); \
    z_pwrite(1, buf, STRLEN(buf), 0); \
  } while(0)

int main()
{
  int i;

  /* memory statistics */
  DUMP("0x%x", MANIFEST->heap_ptr);
  DUMP("0x%x", MANIFEST->heap_size);
  DUMP("0x%x", MANIFEST->stack_size);

  /* channels statistics */
  DUMP("%d\n", MANIFEST->channels_count);
  for(i = 0; i < MANIFEST->channels_count; ++i)
  {
    DUMP("\rchannel #%d", i);

    /* channel name */
    DUMP("%s", MANIFEST->channels[i].name);
    DUMP("%d", MANIFEST->channels[i].type);
    DUMP("%d", MANIFEST->channels[i].size);

    /* channel limits */
    DUMP("%d", MANIFEST->channels[i].limits[0]);
    DUMP("%d", MANIFEST->channels[i].limits[1]);
    DUMP("%d", MANIFEST->channels[i].limits[2]);
    DUMP("%d", MANIFEST->channels[i].limits[3]);
 }

  return 0;
}
