/*
 * this sample demonstrates working with the network channels
 * via file abstraction. specifically this sample copies data
 * from one channel (stdin) to another (stdout). returns 0 if
 * there were no errors.
 *
 * note: to simulate the "real" channel work the program reads
 * "random" amount of bytes
 */
#include <stdlib.h>
#include "include/api_tools.h"

#define CHUNK_SIZE 0x10000
#define SETRAND srand((int)argv[0][6])

int main(int argc, char **argv)
{
  char buffer[CHUNK_SIZE];
  int rsize = 0;
  int wsize = 0;
  int count = 0;
  zvm_bulk = zvm_init();

  UNREFERENCED_VAR(errcount);
  SETRAND;
  ZPRINTF(STDERR, "sranded by %c\n", argv[0][6]);

  /* copy all data from STDIN to STDOUT */
  for(;;)
  {
    count = rand() % sizeof buffer + 1;
    count = zread(STDIN, buffer, count);
    rsize += count;
    ZPRINTF(STDERR, "%5d read, ", count);
    if(count <= 0) break;
    count = zwrite(STDOUT, buffer, count);
    ZPRINTF(STDERR, "%5d written\n", count);
    wsize += count;
  }

  /* report results */
  ZPRINTF(STDERR, "\n");
  if(count < 0)
    ZPRINTF(STDERR, "%s\n", strerror(zvm_errno()));
  ZPRINTF(STDERR, "%d bytes has been read\n", rsize);
  ZPRINTF(STDERR, "%d bytes has been written\n", wsize);

  return rsize == wsize ? 0 : 1;
}
