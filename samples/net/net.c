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

#define FALLING \
"Falling\n"\
"Written by: David Lynch (lyrics), Angelo Badalamenti (music)\n\n"\
"Don't let yourself be hurt this time.\n"\
"Don't let yourself be hurt this time.\n"\
"\n"\
"Then I saw your face\n"\
"Then I saw your smile\n"\
"\n"\
"The sky is still blue\n"\
"The clouds come and go\n"\
"Yet something is different\n"\
"Are we falling in love?\n"\
"\n"\
"Don't let yourself be hurt this time.\n"\
"Don't let yourself be hurt this time.\n"\
"\n"\
"Then your kiss so soft\n"\
"Then your touch so warm\n"\
"\n"\
"The stars still shine bright\n"\
"The mountains still high\n"\
"Yet something is different\n"\
"Are we falling in love?\n"\
"\n"\
"Falling\n"\
"Falling\n"\
"Are we falling in love?\n\n\n\n"

#define CROWD 0x100

int main(int argc, char **argv)
{
//  char message[] = FALLING;
  char message[0x100000];
  char in_message[sizeof message];
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
  for(i = 0; i < sizeof message; ++i)
    message[i] = random();
  
  /* send own name as the 1st message */
  code = fwrite(argv[0], 1, strlen(argv[0]), out);
  fprintf(stderr, "%d bytes has been written of '%s'\n", code, argv[0]);
  
  /* read the 1st message with the name */
  code = fread(in_message, 1, strlen(argv[0]), in);
  fprintf(stderr, "%d bytes has been read of '%s'\n", code, in_message);

  /* main loop */
  for(i = 0; i < CROWD; ++i)
  {
    /* write to "out" (which is connected to "tale" of the net channel) */
    code = fwrite(message, sizeof *message, sizeof message, out);
    fprintf(stderr, "%d iteration: %d bytes has been written of %d\n", i, code, sizeof(message) * sizeof(*message));

    /* read from "in" (which is connected to "head" of the net channel) */
    code = fread(in_message, sizeof *message, sizeof message, in);
    fprintf(stderr, "%d iteration: %d bytes has been read of %d\n", i, code, sizeof(message) * sizeof(*message));
  
    /* print the input message (minus ending '\0') */
    code = fwrite(message, sizeof *message, sizeof message - 1, stdout);
    fprintf(stderr, "%d iteration: %d bytes has been written to stdout\n", i, code);
  }

  fclose(out);
  fclose(in);
  return 0;
}
