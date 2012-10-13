/*
 * test accounting. measure both network and local mounted channels
 * returns the number of failed tests
 *
 * note: not fully functional. network part should be updated in manifest
 */
#include <stdlib.h>
#include "include/api_tools.h"

#define INPUT "INPUT"
#define OUTPUT "OUTPUT"

static uint64_t *acgets;
static uint64_t *acputs;
static uint64_t *acgetsizes;
static uint64_t *acputsizes;

/* allocates memory and resets counters */
static int counters_ctor()
{
  int code;

  acgets = calloc(zvm_bulk->channels_count, sizeof *acgets);
  acputs = calloc(zvm_bulk->channels_count, sizeof *acputs);
  acgetsizes = calloc(zvm_bulk->channels_count, sizeof *acgetsizes);
  acputsizes = calloc(zvm_bulk->channels_count, sizeof *acputsizes);

  /* check allocation */
  if(!acgets || !acputs || !acgetsizes || !acputsizes)
  {
    zput(STDLOG, "cannot allocate counter(s)\n");
    return -1;
  }

  /*
   * patch: since zerovm provides very primitive memory management
   * and calloc doesn't work it is more safe to use memset
   */
  code = zvm_bulk->channels_count * sizeof *acgets;
  memset(acgets, 0, code);
  memset(acputs, 0, code);
  memset(acgetsizes, 0, code);
  memset(acputsizes, 0, code);
  return 0;
}

/* free counters and print accounting data */
static void counters_dtor()
{
  int i;

  for(i = 0; i < zvm_bulk->channels_count; ++i)
  {
    ZPRINTF(STDLOG, "channel %d: gets = %llu, "
        "getsizes = %llu\n", i, acgets[i], acgetsizes[i]);
    ZPRINTF(STDLOG, "channel %d: puts = %llu, "
        "putsizes = %llu\n", i, acputs[i], acputsizes[i]);
  }

  free(acgets);
  free(acputs);
  free(acgetsizes);
  free(acputsizes);
}

/* copies all data from in to out */
static void copy_channel(const char *out, const char *in)
{
  char buf[BIG_ENOUGH];
  int code;

  /* check channels availability */
  ZTEST(zhandle(in) >= 0);
  ZTEST(zhandle(out) >= 0);
  if(zhandle(in) < 0 || zhandle(out) < 0) return;

  /* copy data */
  for(;;)
  {
    code = zread(in, buf, BIG_ENOUGH);
    if(code == 0) break;
    acgetsizes[zhandle(in)] += code;
    ++acgets[zhandle(in)];

    code = zwrite(out, buf, code);
    if(code == 0) break;
    acputsizes[zhandle(out)] += code;
    ++acputs[zhandle(out)];
  }
}

int main(int argc, char **argv)
{
  zvm_bulk = zvm_init();

  /* allocate counters */
  if(counters_ctor() != 0) return 1;

  /* read data from stdin and write to stdout */
  copy_channel(STDOUT, STDIN);

  /* read data from input and write to output */
  copy_channel(OUTPUT, INPUT);

  /* free counters and print accounting data */
  counters_dtor();

  /* count errors and exit with it */
  if(ERRCOUNT > 0)
  ZPRINTF(STDLOG, "TEST FAILED with %d errors\n", ERRCOUNT);
  else zput(STDLOG, "TEST SUCCEED\n\n");

  zvm_exit(ERRCOUNT);
  return 0;
}
