/*
 * this sample demonstrates working with the network channels
 * via file abstraction. specifically this sample copies data
 * from one channel (stdin) to another (stdout). returns 0 if
 * there were no errors.
 *
 * note: to simulate the "real" channel work the program reads
 * "random" amount of bytes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api/zrt.h"

#define CHUNK_SIZE 0x10000

int main(int argc, char **argv)
{
  char buffer[CHUNK_SIZE];
  int rsize = 0;
  int wsize = 0;
  int count = 0;

  srand((int)argv[0][6]);
  fprintf(stderr, "sranded by %c\n", argv[0][6]);

  /* copy all data from stdin to stdout */
  for(;;)
  {
    count = rand() % sizeof buffer + 1;
    count = fread(buffer, sizeof *buffer, count, stdin);
    rsize += count;
    fprintf(stderr, "%5d read, ", count);
    if(count <= 0) break;
    count = fwrite(buffer, sizeof *buffer, count, stdout);
    fprintf(stderr, "%5d written\n", count);
    wsize += count;
  }

  /* report results */
  fprintf(stderr, "\n");
  if(count < 0)
    fprintf(stderr, "%s\n", strerror(zvm_errno()));
  fprintf(stderr, "%d bytes has been read\n", rsize);
  fprintf(stderr, "%d bytes has been written\n", wsize);

  return rsize == wsize ? 0 : 1;
}
