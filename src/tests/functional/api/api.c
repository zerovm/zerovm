/*
 * the zerovm api test
 */
#include <errno.h>
#include "../include/api_tools.h"

/* a cheat to have an access to static members */
#define FailIf(...) /* disable falling */
#include <src/manifest/manifest_parser.c>

#define CONTROL "/dev/control"
#define BIG_ENOUGH 0x10000
#define LOT_ENOUGH 0x100

/*
 * get control data and parse it to make part of manifest parser
 * functions usable: GetValueByKey, GetValuesByKey, ParseValue
 * return 0 if success, otherwise -1
 */
static int manifest_ctor()
{
  static char buf_mft[BIG_ENOUGH];
  static char buf_ptr[BIG_ENOUGH];
  struct ZVMChannel *control = &zvm_bulk->channels[zhandle(CONTROL)];

  /* get the data and initialize manifest */
  mft_count = 0;
  mft_data = buf_mft;
  mft_ptr = (void*)buf_ptr;
  if(zread(CONTROL, mft_data, control->size) < 0) return -1;
  ParseManifest();
  if(mft_count > 0) return 0;

  return -1;
}

/* parse the channel record, find the channel and test its info */
static void test_channel(char *token)
{

}

int main(int argc, char **argv)
{
  zvm_bulk = zvm_init();
  char *tokens[LOT_ENOUGH];
  char *token;
  int number;

  /* get the control data */
  if(manifest_ctor() != 0)
  {
    zput(STDLOG, "cannot get the control data\n");
    zput(STDLOG, "\nTEST FAILED\n");
    zvm_exit(1);
  }

  /* check general information provided by zerovm */
  /* memory */
  token = GetValueByKey("MemMax");
  ZTEST(zvm_bulk->mem_size == (token != NULL ? atoi(token) : 0));

  /* environment. todo(d'b): extend it */
  token = GetValueByKey("Environment");
  if(token == NULL)
    ZTEST(zvm_bulk->envp == NULL);
  else
    ZTEST(zvm_bulk->envp != NULL);

  /* node name */
  token = GetValueByKey("NodeName");
  if(token != NULL)
  {
    number = ParseValue(token, ", ", tokens, 2);
    ZTEST(number == 2);
    ZTEST(strcmp(argv[0], tokens[0]) == 0);
  }
  else
    ZTEST(strcmp(argv[0], "loner") == 0);

  /* command line */
  token = GetValueByKey("CommandLine");
  if(token != NULL)
  {
    int i;
    number = ParseValue(token, ", ", tokens, LOT_ENOUGH);
    ZTEST(argc == number + 1);
    for(i = 0; i < number; ++i)
      ZTEST(strcmp(argv[i+1], tokens[i]) == 0);
    ZTEST(argv[number + 1] == NULL);
  }
  else
    ZTEST(argv[1] == NULL);

  /* channels */
  number = GetValuesByKey("Channel", tokens, LOT_ENOUGH);
  ZTEST(zvm_bulk->channels_count == number);

  /* check channels one by one */
  for(number = 0; number < zvm_bulk->channels_count; ++number)
    test_channel(tokens[number]);

  /* check stdin information */
  ZTEST(strcmp(zvm_bulk->channels[0].name, STDIN) == 0);
  ZTEST(zvm_bulk->channels[0].limits[GetsLimit] == 256);
  ZTEST(zvm_bulk->channels[0].limits[GetSizeLimit] == 16384);
  ZTEST(zvm_bulk->channels[0].limits[PutsLimit] == 0);
  ZTEST(zvm_bulk->channels[0].limits[PutSizeLimit] == 0);
  ZTEST(zvm_bulk->channels[0].type == 0);

  /* check stdout information */
  ZTEST(strcmp(zvm_bulk->channels[1].name, STDOUT) == 0);
  ZTEST(zvm_bulk->channels[1].limits[GetsLimit] == 0);
  ZTEST(zvm_bulk->channels[1].limits[GetSizeLimit] == 0);
  ZTEST(zvm_bulk->channels[1].limits[PutsLimit] == 1024);
  ZTEST(zvm_bulk->channels[1].limits[PutSizeLimit] == 65536);
  ZTEST(zvm_bulk->channels[1].type == 0);

  /* check stderr information */
  ZTEST(strcmp(zvm_bulk->channels[2].name, STDERR) == 0);
  ZTEST(zvm_bulk->channels[2].limits[GetsLimit] == 0);
  ZTEST(zvm_bulk->channels[2].limits[GetSizeLimit] == 0);
  ZTEST(zvm_bulk->channels[2].limits[PutsLimit] == 512);
  ZTEST(zvm_bulk->channels[2].limits[PutSizeLimit] == 32768);
  ZTEST(zvm_bulk->channels[2].type == 0);

  /* check control channel information */
  ZTEST(strcmp(zvm_bulk->channels[3].name, CONTROL) == 0);
  ZTEST(zvm_bulk->channels[3].limits[GetsLimit] == 128);
  ZTEST(zvm_bulk->channels[3].limits[GetSizeLimit] == 8192);
  ZTEST(zvm_bulk->channels[3].limits[PutsLimit] == 0);
  ZTEST(zvm_bulk->channels[3].limits[PutSizeLimit] == 0);
  ZTEST(zvm_bulk->channels[3].type == 1);
  ZTEST(zvm_bulk->channels[3].size == 864);

  /* exit with code */
  zput(STDLOG, ERRCOUNT ? "\nTEST FAILED\n" : "\nTEST SUCCEED\n");
  zvm_exit(0);

  /* not reached */
  return 0;
}
