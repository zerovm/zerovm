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
#include "/home/dazo/git/zerovm/api/zvm_manifest.h"

#define CHUNK_SIZE 0x10000000 /* 256mb */
#define LEAVE(a, c) {free(a); return (c);}

int main(int argc, char **argv)
{
  int64_t position = 0;
  int64_t insize;
  int64_t outsize;
  char *buffer = NULL;

  /* allocate buffer */
  if((buffer = (char*) malloc(CHUNK_SIZE)) == NULL) LEAVE(buffer, -1);

  /* copy input channel to output channel */
  while((insize = zvm_pread(InputChannel, buffer, CHUNK_SIZE, position)) > 0)
  {
    outsize = zvm_write(OutputChannel, buffer, insize, position);
    fprintf(stderr, "read [%lld] bytes, written %lld bytes\n", insize, outsize);
    if(insize != outsize) LEAVE(buffer, -2); /* check if some bytes lost in action */
    position += insize;
    fprintf(stderr, "position = %lld\n", position);
  }

  LEAVE(buffer, 0)
}
