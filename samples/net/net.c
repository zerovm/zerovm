/*
 * this sample demonstrates working with the network channels
 * via file abstraction
 *
 * in order to use zrt "zrt.h" should be included
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "zrt.h"

#define CHUNK_COUNT 0x10
#define CHUNK_SIZE 0x100000
#define INFILE "/dev/in/in"
#define OUTFILE "/dev/out/out"
#define SIGNATURE argv[1][0]

int main(int argc, char **argv)
{
  char message[CHUNK_SIZE];
  char in_message[CHUNK_SIZE];
  FILE *in, *out;
  int i;
  int code;
  
  in = fopen(INFILE, "r");
  out = fopen(OUTFILE, "w");

  if(in == NULL || out == NULL) 
  {
    fprintf(stderr, "couldn't open i/o file(s)\n");
    return 1;
  }

  /* initialize the array */
  for(i = 0; i < CHUNK_SIZE; ++i)
    message[i] = SIGNATURE;

  /* send own name as the 1st message */
  code = fwrite(argv[0], 1, strlen(argv[0]), out);
  fprintf(stderr, "written %d bytes: '%s'\n", code, argv[0]);
  fflush(stderr);

  /* read the 1st message with the name */
  code = fread(in_message, 1, strlen(argv[0]), in);
  fprintf(stderr, "read %d bytes: '%s'\n", code, in_message);
  fflush(stderr);

  /* main loop */
  for(i = 0; i < CHUNK_COUNT; ++i)
  {
    /* write to "out" (which is connected to "tale" of the net channel) */
    code = fwrite(message, 1, CHUNK_SIZE, out);
    fprintf(stderr, "%s [%d]: %d of %d bytes written\n",
      argv[0], i, code, CHUNK_SIZE * sizeof(*message));
    fflush(stderr);
      
    /* read from "in" (which is connected to "head" of the net channel) */
    code = fread(in_message, 1, CHUNK_SIZE, in);
    fprintf(stderr, "%s [%d]: %d of %d bytes read\n",
      argv[0], i, code, CHUNK_SIZE * sizeof(*message));
    fflush(stderr);

    /* log the input message */
    fwrite(in_message, 1, CHUNK_SIZE, stdout);
  }

  fclose(out);
  fclose(in);
  return 0;
}
