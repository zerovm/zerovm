/*
 * the zerovm api test
 * note: zvm api functions not tested for invalid arguments
 * since zerovm doesn't have user buffer validation. this issue
 * will be fixed either if check routine will be added to zvm
 * or if memory management will be completely moved to untrusted code
 * (in that case zvm will just need to check if pointer is in range)
 */
#include <errno.h>
#include <glib.h>
#include "include/api_tools.h"

/*
 * avoid compile errors (gnome library isn't ported to nacl)
 * because the code uses c-source inclusion below
 */
#define g_malloc malloc
#define g_malloc0 my_calloc
#define g_free free

int64_t g_ascii_strtoll();

/* single argument calloc version */
void *my_calloc(size_t size)
{
  return calloc(1, size);
}

/* a cheat to have an access to static members */
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

/* disable falling */
#undef ZLOGFAIL
#define ZLOGFAIL(...)
void ZLogTag(const char *file, int line) {}
void ZLog(int priority, char *fmt, ...) {}
void FailIf(int cond, int err, char const *fmt, ...) {}

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
  char *tokens[LOT_ENOUGH];
  int i;

  /* show channel control info */
  zput(STDLOG, "the testing channel control info: ");
  zput(STDLOG, token);
  zput(STDLOG, "\n");

  /* parse token */
  i = ParseValue(token, ", ", tokens, LOT_ENOUGH);
  ZTEST(i == CHANNEL_ATTRIBUTES_NUMBER);

  /* find appropriate channel */
  i = zhandle(tokens[CFieldAlias]);

  /* compare channel attributes with the control ones */
  ZTEST(strcmp(zvm_bulk->channels[i].name, tokens[CFieldAlias]) == 0);
  ZTEST(zvm_bulk->channels[i].limits[GetsLimit] == atoi(tokens[CFieldGetsLimit]));
  ZTEST(zvm_bulk->channels[i].limits[GetSizeLimit] == atoi(tokens[CFieldGetSizeLimit]));
  ZTEST(zvm_bulk->channels[i].limits[PutsLimit] == atoi(tokens[CFieldPutsLimit]));
  ZTEST(zvm_bulk->channels[i].limits[PutSizeLimit] == atoi(tokens[CFieldPutSizeLimit]));
  ZTEST(zvm_bulk->channels[i].type == atoi(tokens[CFieldType]));
}

int main(int argc, char **argv)
{
  zvm_bulk = zvm_init();
  char *tokens[LOT_ENOUGH];
  char *token;
  int number;

  UNREFERENCED_FUNCTION(g_ascii_strtoll);

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
  ZTEST((uintptr_t)zvm_bulk->heap_ptr + zvm_bulk->heap_size + STACK_SIZE
      == (token != NULL ? atoi(token) : 0));

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

  /* check channels number */
  number = GetValuesByKey("Channel", tokens, LOT_ENOUGH);
  ZTEST(zvm_bulk->channels_count == number);

  /* test channels one by one */
  for(number = 0; number < zvm_bulk->channels_count; ++number)
    test_channel(tokens[number]);

  /* count errors and exit with it */
  if(ERRCOUNT > 0)
    ZPRINTF(STDLOG, "TEST FAILED with %d errors\n", ERRCOUNT);
  else
    zput(STDLOG, "TEST SUCCEED\n\n");

  zvm_exit(ERRCOUNT);
  return 0;
}
