/*
 * invokes the signal specified by the 1st parameter of the command line
 * supported signals: 8, 11, 14, 25
 * todo(d'b): add signals
 * update: it is possible to invoke almost any signal from makefile via pkill.
 *         it would be better to initiate signal from the user code, but zerovm
 *         can be also tested from outside
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

#define DEFAULT_SIGNAL 11
#define SIGNALS_NUMBER 64
#define BIGFILE "quota.data"

void signal_na()
{
  FPRINTF(STDERR, "not supported signal\n");
}

/* Floating point exception */
void signal_8()
{
  int i = 0;
  FPRINTF(STDERR, "invocation of signal #8\n");
  FPRINTF(STDERR, "falling.. %d\n", 1/i);
}

/* Invalid memory reference */
void signal_11()
{
  FPRINTF(STDERR, "invocation of signal #11\n");
  FPRINTF(STDERR, "falling.. %d\n", *(char*)NULL);
}

/* CPU time limit exceeded */
void signal_14()
{
  FPRINTF(STDERR, "invocation of signal #14\n");
  for(;;);
}

/* disk i/o limit exceeded */
#define MEGABYTE 0x100000
void signal_25()
{
  char buf[MEGABYTE];
  FPRINTF(STDERR, "invocation of signal #25\n");

  for(;;)
    WRITE(BIGFILE, buf, MEGABYTE);
}

/* initialize array of pointers to signal raisers */
void signals_ctor(void (*signals[])())
{
  int i;

  for(i = 0; i < SIGNALS_NUMBER; ++i)
    signals[i] = signal_na;

  signals[8] = signal_8;
  signals[11] = signal_11;
  signals[14] = signal_14;
  signals[25] = signal_25;
}

int main(int argc, char **argv, char **envp)
{
  void (*signals[SIGNALS_NUMBER])();
  uint32_t signum = DEFAULT_SIGNAL;

  /* constructor of signal raisers */
  if(argc == 2)
    signum = STRTOL(argv[1], NULL, 10);
  signals_ctor(signals);

  /* provoke the signal */
  if(signum < SIGNALS_NUMBER)
    signals[signum]();

  /* not reached if the signal handler invoked */
  FPRINTF(STDERR, "FAILED on signal #%u\n", signum);
  return ERRCOUNT;
}
