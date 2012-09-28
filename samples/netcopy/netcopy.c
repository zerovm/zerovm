/*
 * this sample demonstrates working with the network channels
 * via file abstraction. specifically this sample copies data
 * from one channel (stdin) to another (stdout). returns 0 if
 * there were no errors.
 */
#include <stdio.h>
#include "api/zrt.h"

#define CHUNK_SIZE 0x10000

int main(int argc, char **argv)
{
  char buffer[CHUNK_SIZE];
  int rsize = 0;
  int wsize = 0;
  int count = 1;

  /* copy all data from stdin to stdout */
  while(count != 0)
  {
    count = fread(buffer, sizeof *buffer, sizeof buffer, stdin);
    wsize += fwrite(buffer, sizeof *buffer, count, stdout);
    rsize += count;
  }

  fprintf(stderr, "%d bytes has bean read\n", rsize);
  fprintf(stderr, "%d bytes has bean written\n", wsize);

  return rsize == wsize ? 0 : 1;
}
