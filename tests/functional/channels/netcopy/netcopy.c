/*
 * this sample demonstrates working with the network channels
 * via file abstraction. specifically this sample copies data
 * from one channel (stdin) to another (stdout). returns 0 if
 * there were no errors.
 *
 * note: to simulate the "real" channel work the program reads
 * "random" amount of bytes
 *
 * update: to test 0mq put/get the program tries to slow down
 * twice each next node
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

#define CHUNK_SIZE 0x10000

/* since zerovm modules does not have a real sleep */
#define LOOP_SIZE 0x100000LLU
static void busy_loop(int factor)
{
  static uint64_t dummy = 0;
  uint64_t loop = LOOP_SIZE * factor + 1;
  while(--loop)
    dummy *= loop;
}

int main(int argc, char **argv)
{
  int rsize = 0;
  int wsize = 0;
  int factor = atoi(argv[0] + 6);

  UNREFERENCED_VAR(ERRCOUNT);
  SRAND(factor);
  FPRINTF(STDERR, "seeded with %d\n", factor);

  /* copy all data from STDIN to STDOUT */
  for(;;)
  {
    char buffer[CHUNK_SIZE];
    int plan = RAND() % sizeof buffer + 1;
    int count = READ(STDIN, buffer, plan);

    busy_loop(factor);

    /* read error */
    if(count < 0)
    {
      FPRINTF(STDERR, "read error %d\n", count);
      break;
    }

    /* eof */
    if(count == 0) break;

    rsize += count;
    FPRINTF(STDERR, "%5d(of %5d) read, ", count, plan);

    count = WRITE(STDOUT, buffer, count);

    /* write error */
    if(count < 0)
    {
      FPRINTF(STDERR, "write error %d\n", count);
      break;
    }

    FPRINTF(STDERR, "%5d written\n", count);
    wsize += count;
  }

  /* report results */
  FPRINTF(STDERR, "\n");
  FPRINTF(STDERR, "%d bytes has been read\n", rsize);
  FPRINTF(STDERR, "%d bytes has been written\n", wsize);

  return rsize == wsize ? 0 : 1;
}
