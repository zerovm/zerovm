/*
 * invokes signal
 */
#include <signal.h>
#include "include/api_tools.h"

#define SIGNALS_NUMBER
#define RAISER 8

void signal_na()
{
  ZPRINTF(STDOUT, "this signal not supported\n");
}

void signal_8()
{
  int i = 0;
  ZPRINTF(STDOUT, "invocation of signal #8\n");
  ZPRINTF(STDOUT, "falling.. %d\n", 1/i);
}

void signal_11()
{
  ZPRINTF(STDOUT, "invocation of signal #11\n");
  ZPRINTF(STDOUT, "falling.. %d\n", *(char*)NULL);
}

/* initialize array of pointers to signal raisers */
void signals_ctor(void (*raisers[])())
{
  raisers[8] = signal_8;
  raisers[11] = signal_11;
}

int main()
{
  zvm_bulk = zvm_init();
  void (*raisers[])()  = {signal_na};

  UNREFERENCED_VAR(errcount);

  /* constructor of signal raisers */
  signals_ctor(raisers);

  /* provoke the signal */
  raisers[RAISER]();

  /* not reached */
  ZPRINTF(STDOUT, "must not seen\n");
  return 0;
}
