/*
 * the zerovm api test
 */
#include <errno.h>
#include "../include/api_tools.h"

#define STDLOG STDOUT
#define CONTROL "/dev/control"
/*
 * if condition is false increase the errors count,
 * log result to log channel (defined above)
 */
#define COND_TRUE(cond) \
  do {\
    char msg[0x100];\
    if(cond)\
    {\
      snprintf(msg, 0x100, "succeed on %3d: %s\n", __LINE__, #cond);\
      zput(STDLOG, msg);\
    }\
    else\
    {\
      snprintf(msg, 0x100, "failed on %4d: %s\n", __LINE__, #cond);\
      zput(STDLOG, msg);\
      ++result;\
    }\
  } while(0)

int main(int argc, char **argv)
{
  int result = 0;
  zvm_bulk = zvm_init();

  /* check general information provided by zerovm */
  COND_TRUE(zvm_bulk->channels_count == 4);
  COND_TRUE(zvm_bulk->heap_ptr != NULL);
  COND_TRUE(zvm_bulk->mem_size == 268435456);
  COND_TRUE(zvm_bulk->envp == NULL);
  COND_TRUE(argc == 1);
  COND_TRUE(strcmp(argv[0], "api") == 0);
  COND_TRUE(argv[1] == NULL);

  /* check stdin information */
  COND_TRUE(strcmp(zvm_bulk->channels[0].name, STDIN) == 0);
  COND_TRUE(zvm_bulk->channels[0].limits[GetsLimit] == 256);
  COND_TRUE(zvm_bulk->channels[0].limits[GetSizeLimit] == 16384);
  COND_TRUE(zvm_bulk->channels[0].limits[PutsLimit] == 0);
  COND_TRUE(zvm_bulk->channels[0].limits[PutSizeLimit] == 0);
  COND_TRUE(zvm_bulk->channels[0].type == 0);

  /* check stdout information */
  COND_TRUE(strcmp(zvm_bulk->channels[1].name, STDOUT) == 0);
  COND_TRUE(zvm_bulk->channels[1].limits[GetsLimit] == 0);
  COND_TRUE(zvm_bulk->channels[1].limits[GetSizeLimit] == 0);
  COND_TRUE(zvm_bulk->channels[1].limits[PutsLimit] == 1024);
  COND_TRUE(zvm_bulk->channels[1].limits[PutSizeLimit] == 65536);
  COND_TRUE(zvm_bulk->channels[1].type == 0);

  /* check stderr information */
  COND_TRUE(strcmp(zvm_bulk->channels[2].name, STDERR) == 0);
  COND_TRUE(zvm_bulk->channels[2].limits[GetsLimit] == 0);
  COND_TRUE(zvm_bulk->channels[2].limits[GetSizeLimit] == 0);
  COND_TRUE(zvm_bulk->channels[2].limits[PutsLimit] == 512);
  COND_TRUE(zvm_bulk->channels[2].limits[PutSizeLimit] == 32768);
  COND_TRUE(zvm_bulk->channels[2].type == 0);

  /* check control channel information */
  COND_TRUE(strcmp(zvm_bulk->channels[3].name, CONTROL) == 0);
  COND_TRUE(zvm_bulk->channels[3].limits[GetsLimit] == 128);
  COND_TRUE(zvm_bulk->channels[3].limits[GetSizeLimit] == 8192);
  COND_TRUE(zvm_bulk->channels[3].limits[PutsLimit] == 0);
  COND_TRUE(zvm_bulk->channels[3].limits[PutSizeLimit] == 0);
  COND_TRUE(zvm_bulk->channels[3].type == 1);

  /* exit with code */
  zput(STDLOG, result ? "\nTEST FAILED\n" : "\nTEST SUCCEED\n");
  zvm_exit(0);

  /* not reached */
  return 0;
}
