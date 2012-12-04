/*
 * invokes the signal specified by the 1st parameter of the command line
 * supported signals: 8, 11, 24
 * todo(d'b): add signals
 *   2 - Interrupt from keyboard
 *   25 - File size limit exceeded
 */
#include <stdlib.h>
#include "include/api_tools.h"

#define SIGNALS_NUMBER 64

void signal_na()
{
  ZPRINTF(STDOUT, "not supported signal\n");
}

/* Floating point exception */
void signal_8()
{
  int i = 0;
  ZPRINTF(STDOUT, "invocation of signal #8\n");
  ZPRINTF(STDOUT, "falling.. %d\n", 1/i);
}

/* Invalid memory reference */
void signal_11()
{
  ZPRINTF(STDOUT, "invocation of signal #11\n");
  ZPRINTF(STDOUT, "falling.. %d\n", *(char*)NULL);
}

/* CPU time limit exceeded */
void signal_24()
{
  ZPRINTF(STDOUT, "invocation of signal #24\n");
  for(;;);
}

/* initialize array of pointers to signal raisers */
void signals_ctor(void (*signals[])())
{
  int i;

  for(i = 0; i < SIGNALS_NUMBER; ++i)
    signals[i] = signal_na;

  signals[8] = signal_8;
  signals[11] = signal_11;
  signals[24] = signal_24;
}

int main(int argc, char **argv, char **envp)
{
  zvm_bulk = zvm_init();
  void (*signals[SIGNALS_NUMBER])();
  uint32_t signum = -1U;

  UNREFERENCED_VAR(errcount);

  /* constructor of signal raisers */
  signals_ctor(signals);

  /* provoke the signal */
  if(argc == 2)
  {
    signum = strtol(argv[1], NULL, 10);
    if(signum < SIGNALS_NUMBER)
      signals[signum]();
  }

  /* not reached if the signal handler invoked */
  ZPRINTF(STDOUT, "FAILED on signal #%u\n", signum);
  return 0;
}
