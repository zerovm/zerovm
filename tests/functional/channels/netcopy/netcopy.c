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

#define CHUNK_SIZE 0x100000
#define BREAKIF(cond, ...) if(cond) {FPRINTF(STDERR, __VA_ARGS__); break;}

/* zerovm platform does not have a real sleep */
#define LOOP_SIZE 0x20000LLU
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

  SRAND(factor);
  FPRINTF(STDERR, "seeded with %d\n", factor);

  /* copy all data from STDIN to STDOUT */
  for(;;)
  {
    char buffer[CHUNK_SIZE];
//    int plan = RAND() % sizeof buffer + 1;
    int plan = (rand() * rand()) % sizeof buffer + 1;

    /* read */
    int count = READ(STDIN, buffer, plan);
    BREAKIF(count < 0, "read error %d\n", count);
    busy_loop(factor);

    /* eof */
    if(count == 0) break;

    /* the read log */
    rsize += count;
    FPRINTF(STDERR, "%5d(of %5d) read, ", count, plan);

    /* write */
    count = WRITE(STDOUT, buffer, count);
    BREAKIF(count < 0, "write error %d\n", count);

    /* the write log */
    FPRINTF(STDERR, "%5d written\n", count);
    wsize += count;
  }

  /* report results */
  FPRINTF(STDERR, "\n%d bytes has been read\n", rsize);
  FPRINTF(STDERR, "%d bytes has been written\n", wsize);

  return rsize == wsize ? 0 : 1;
}
