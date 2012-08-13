/*
 * this sample demonstrates working with the network channels
 * via file abstraction
 *
 * in order to use zrt "api/zrt.h" should be included
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "api/zrt.h"

#define CHUNK_COUNT 0x2
#define CHUNK_SIZE 0x100000

int main(int argc, char **argv)
{
  char message[CHUNK_SIZE];
  char in_message[CHUNK_SIZE];
  FILE *in, *out;
  int i;
  int code;
  
  in = fopen("in", "r");
  out = fopen("out", "w");

  if(in == NULL || out == NULL) 
  {
    fprintf(stderr, "couldn't open i/o file(s)\n");
    return 1;
  }

  /* initialize the array */
  srand(time(0));
  fprintf(stderr, "time = %d\n", (int)time(0));
  for(i = 0; i < CHUNK_SIZE; ++i)
    message[i] = random();
  
  /* send own name as the 1st message */
  code = fwrite(argv[0], 1, strlen(argv[0]), out);
  fprintf(stderr, "%d bytes has been written of '%s'\n", code, argv[0]);
  
  /* read the 1st message with the name */
  code = fread(in_message, 1, strlen(argv[0]), in);
  fprintf(stderr, "%d bytes has been read of '%s'\n", code, in_message);

  /* main loop */
  for(i = 0; i < CHUNK_COUNT; ++i)
  {
    /* write to "out" (which is connected to "tale" of the net channel) */
    code = fwrite(message, 1, CHUNK_SIZE, out);
    fprintf(stderr, "%d iteration: %d bytes has been written of %d\n", 
      i, code, CHUNK_SIZE * sizeof(*message));

    /* read from "in" (which is connected to "head" of the net channel) */
    code = fread(in_message, 1, CHUNK_SIZE, in);
    fprintf(stderr, "%d iteration: %d bytes has been read of %d\n", 
      i, code, CHUNK_SIZE * sizeof(*message));
  
    /* log the input message */
    fwrite(in_message, 1, CHUNK_SIZE, stdout);
  }

  fclose(out);
  fclose(in);
  return 0;
}
