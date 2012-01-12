/*
 * test for pagination engine
 *
 *  Created on: Dec 12, 2011
 *      Author: d'b
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "api/zvm.h"

#define CHUNK_SIZE 0x10000000 /* 256mb */
#define LEAVE(a, c) {free(a); return (c);}

#define LOGFIX /* temporary fix until zrt library will be finished */

#ifdef LOGFIX
#define fprintf(ch, ...)\
do {\
  char msg[4096];\
  sprintf(msg, __VA_ARGS__);\
  log_msg(msg);\
} while (0)
#endif

int main(int argc, char **argv)
{
  int64_t position = 0;
  int64_t insize;
  int64_t outsize;
  char *buffer = NULL;

#ifdef LOGFIX
  /* set up the log */
  int retcode = ERR_CODE;
  struct SetupList setup;
  retcode = zvm_setup(&setup);
  retcode = log_set(&setup);
  if(retcode) return retcode;
#endif

  /* allocate buffer */
  if((buffer = (char*) malloc(CHUNK_SIZE)) == NULL) LEAVE(buffer, -1);

  /* copy input channel to output channel */
  while((insize = zvm_pread(InputChannel, buffer, CHUNK_SIZE, position)) > 0)
  {
    outsize = zvm_pwrite(OutputChannel, buffer, insize, position);
    fprintf(stderr, "read [%lld] bytes, written %lld bytes\n", insize, outsize);
    if(insize != outsize) LEAVE(buffer, -2); /* check if some bytes lost in action */
    position += insize;
    fprintf(stderr, "position = %lld\n", position);
  }

  LEAVE(buffer, 0)
  return 0; /* not reachable */
}
