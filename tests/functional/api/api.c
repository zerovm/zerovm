/*
 * the zerovm api test
 * note: zvm api functions not tested for invalid arguments
 * since zerovm doesn't have user buffer validation. this issue
 * will be fixed either if check routine will be added to zvm
 * or if memory management will be completely moved to untrusted code
 * (in that case zvm will just need to check if pointer is in range)
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

#define LOT_ENOUGH 0x100

/* prepare everything to cheat "manifest_parser.c" */
/* skip "extra load" headers */
#define TOOLS_H_ 1
#define __G_LIB_H__ 1
#define _ERRNO_H 1
#define _CTYPE_H 1

/* disable falling */
#undef ZLOGFAIL
#define ZLOGFAIL(...)
#define ZLogTag(...)
#define ZLog(...)
#define FailIf(...)
#define fopen(...) NULL
#define fread(...) -1
#define fclose(...)
#define GetFileSize(...) -1

/* redefine functions */
#define g_malloc MALLOC
#define g_malloc0(size) CALLOC(size, 1)
#define g_free FREE

/* a cheat to have an access to static members */
#define NDEBUG /* to disable assert() */
#include "src/main/manifest_parser.c"

#define CONTROL "/dev/control"
#define CHANNEL_ATTRIBUTES_NUMBER 7
enum ChannelFields {
  CFieldURI,
  CFieldAlias,
  CFieldType,
  CFieldGetsLimit,
  CFieldGetSizeLimit,
  CFieldPutsLimit,
  CFieldPutSizeLimit
};

/*
 * get control data and parse it to make part of manifest parser
 * functions usable: GetValueByKey, GetValuesByKey, ParseValue
 * return 0 if success, otherwise -1
 */
static int manifest_ctor()
{
  static char buf_mft[BIG_ENOUGH];
  static char buf_ptr[BIG_ENOUGH];

  /* get the data and initialize manifest */
  mft_count = 0;
  mft_data = buf_mft;
  mft_ptr = (void*)buf_ptr;

  if(READ(CONTROL, mft_data, MANIFEST->channels[OPEN(CONTROL)].size) < 0)
    return -1;

  ParseManifest();
  if(mft_count > 0)
    return 0;

  return -1;
}

/* parse the channel record, find the channel and test its info */
static void test_channel(char *token)
{
  char *tokens[LOT_ENOUGH];
  int i;

  /* show channel control info */
  FPRINTF(STDERR, "the testing channel control info: ");
  FPRINTF(STDERR, token);
  FPRINTF(STDERR, "\n");

  /* parse token */
  i = ParseValue(token, ", ", tokens, LOT_ENOUGH);
  ZTEST(i == CHANNEL_ATTRIBUTES_NUMBER);

  /* find appropriate channel */
  i = OPEN(tokens[CFieldAlias]);

  /* compare channel attributes with the control ones */
  ZTEST(STRCMP(MANIFEST->channels[i].name, tokens[CFieldAlias]) == 0);
  ZTEST(MANIFEST->channels[i].limits[GetsLimit] == ATOI(tokens[CFieldGetsLimit]));
  ZTEST(MANIFEST->channels[i].limits[GetSizeLimit] == ATOI(tokens[CFieldGetSizeLimit]));
  ZTEST(MANIFEST->channels[i].limits[PutsLimit] == ATOI(tokens[CFieldPutsLimit]));
  ZTEST(MANIFEST->channels[i].limits[PutSizeLimit] == ATOI(tokens[CFieldPutSizeLimit]));
  ZTEST(MANIFEST->channels[i].type == ATOI(tokens[CFieldType]));
}

int main(int argc, char **argv)
{
  char *tokens[LOT_ENOUGH];
  char *token;
  int number;

  /* get the control data */
  if(manifest_ctor() != 0)
  {
    FPRINTF(STDERR, "cannot get the control data\n");
    FPRINTF(STDERR, "\nTEST FAILED\n");
    return 1;
  }

  /* check general information provided by zerovm */
  /* memory */
  token = GetValueByKey("MemMax");
  ZTEST((uintptr_t)MANIFEST->heap_ptr + MANIFEST->heap_size + MANIFEST->stack_size
      == (token != NULL ? ATOI(token) : 0));

  /* node name */
  token = GetValueByKey("NodeName");
  if(token != NULL)
  {
    number = ParseValue(token, ", ", tokens, 2);
    ZTEST(number == 2);
    ZTEST(STRCMP(argv[0], tokens[0]) == 0);
  }
  else
    ZTEST(STRCMP(argv[0], "loner") == 0);

  /* check channels number */
  number = GetValuesByKey("Channel", tokens, LOT_ENOUGH);
  ZTEST(MANIFEST->channels_count == number);

  /* test channels one by one */
  for(number = 0; number < MANIFEST->channels_count; ++number)
    test_channel(tokens[number]);

  /* count errors and exit with it */
  if(ERRCOUNT > 0)
    FPRINTF(STDERR, "TEST FAILED with %d errors\n", ERRCOUNT);
  else
    FPRINTF(STDERR, "TEST SUCCEED\n\n");

  return ERRCOUNT;
}
