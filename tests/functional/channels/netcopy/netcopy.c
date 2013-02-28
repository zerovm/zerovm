/*
 * this sample demonstrates working with the network channels
 * via file abstraction. specifically this sample copies data
 * from one channel (stdin) to another (stdout). returns 0 if
 * there were no errors.
 *
 * note: to simulate the "real" channel work the program reads
 * "random" amount of bytes
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

#define CHUNK_SIZE 0x10000

int main(int argc, char **argv)
{
  char buffer[CHUNK_SIZE];
  int rsize = 0;
  int wsize = 0;
  int count = 0;

  UNREFERENCED_VAR(ERRCOUNT);
  SRAND((int)argv[0][6]);
  FPRINTF(STDERR, "seeded with %d\n", (int)argv[0][6]);

  /* copy all data from STDIN to STDOUT */
  for(;;)
  {
    count = RAND() % sizeof buffer + 1;
    count = READ(STDIN, buffer, count);

    /* read error */
    if(count < 0)
    {
      FPRINTF(STDERR, "read error %d\n", count);
      break;
    }

    /* eof */
    if(count == 0) break;

    rsize += count;
    FPRINTF(STDERR, "%5d read, ", count);

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
