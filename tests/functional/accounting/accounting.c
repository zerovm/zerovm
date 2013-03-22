/*
 * under construction
 * test accounting. measure both network and local mounted channels
 * returns the number of failed tests
 *
 * note: not fully functional. network part should be updated in manifest
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

static uint64_t *acgets;
static uint64_t *acputs;
static uint64_t *acgetsizes;
static uint64_t *acputsizes;

/* allocates memory and resets counters */
static int counters_ctor()
{
  acgets = CALLOC(MANIFEST->channels_count, sizeof *acgets);
  acputs = CALLOC(MANIFEST->channels_count, sizeof *acputs);
  acgetsizes = CALLOC(MANIFEST->channels_count, sizeof *acgetsizes);
  acputsizes = CALLOC(MANIFEST->channels_count, sizeof *acputsizes);

  /* check allocation */
  ZFAIL(acgets && acputs && acgetsizes && acputsizes);

  return 0;
}

/* free counters and print accounting data */
static void counters_dtor()
{
  int i;

  for(i = 0; i < MANIFEST->channels_count; ++i)
  {
    FPRINTF(STDERR, "channel %d: gets = %u, "
        "getsizes = %u\n", i, acgets[i], acgetsizes[i]);
    FPRINTF(STDERR, "channel %d: puts = %u, "
        "putsizes = %u\n", i, acputs[i], acputsizes[i]);
  }

  FREE(acgets);
  FREE(acputs);
  FREE(acgetsizes);
  FREE(acputsizes);
}

/* copies all data from in to out */
static void copy_channel(const char *out, const char *in)
{
  char buf[BIG_ENOUGH];
  int code;

  /* check channels availability */
  ZFAIL(OPEN(in) >= 0);
  ZFAIL(OPEN(out) >= 0);

  /* copy data */
  for(;;)
  {
    code = READ(in, buf, BIG_ENOUGH);
    if(code == 0) break;
    acgetsizes[OPEN(in)] += code;
    ++acgets[OPEN(in)];

    code = WRITE(out, buf, code);
    if(code == 0) break;
    acputsizes[OPEN(out)] += code;
    ++acputs[OPEN(out)];
  }
}

int main(int argc, char **argv)
{
  /* allocate counters */
  ZTEST(counters_ctor() == 0);

  /* read data from stdin and write to stdout */
  copy_channel(STDOUT, STDIN);

  /* temporary disabled */
#if 0
  /* read data from input and write to output */
  copy_channel(OUTPUT, INPUT);
#endif

  /* free counters and print accounting data */
  counters_dtor();

  /* count errors and exit with it */
  ZREPORT;
  return 0; /* prevent warning */
}
