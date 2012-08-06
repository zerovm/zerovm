/*
 * this sample demonstrates working with the network channels
 * via file abstraction
 *
 * in order to use zrt "api/zrt.h" should be included
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#define CROWD 0x1000

int main(int argc, char **argv)
{
  char message[] = FALLING;
//  char message[0x100000];
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
  
  /* populate buffer with values */
  /*
  for(i = 0; i < sizeof message; ++i)
    message[i] = (char)random();
  */
  
  for(i = 0; i < CROWD; ++i)
  {
    /* write to stdout (which is connected to "tale" of the net channel) */
    code = fwrite(message, sizeof *message, sizeof message, out);
    fprintf(stderr, "%d iteration: %d bytes has been written of %d\n", i, code, sizeof(message) * sizeof(*message));

    /* read from stdin (which is connected to "head" of the net channel) */
    code = fread(in_message, sizeof *message, sizeof message, in);
    fprintf(stderr, "%d iteration: %d bytes has been read of %d\n", i, code, sizeof(message) * sizeof(*message));
  
    /* print the input message (minus ending '\0') */
    code = fwrite(message, sizeof *message, sizeof message - 1, stdout);
    fprintf(stderr, "%d iteration: %d bytes has been written to stdout\n", i, code);
  }
  
  fclose(in);
  fclose(out);

  return 0;
}
