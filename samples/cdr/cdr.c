/*
 * cdr channel demostration. the cdr channel should have data
 *
 * in order to use zrt "zrt.h" should be included
 */
#include <stdio.h>
#include <string.h>
#include "zrt.h"

#define BUF_SIZE 0x1000
#define APP_STRING "hello, world\n"

int main(int argc, char **argv)
{
  FILE *cdr = fopen("/dev/cdr", "a");
  char buffer[BUF_SIZE];
  int code;

  /* check for a file open error */
  if(cdr == NULL)
  {
    fprintf(stderr, "cdr open error\n");
    return 1;
  }

  /* write to cdr. string will be append to the cdr end */
  code = fputs(APP_STRING, cdr);
  fprintf(stderr, "fputs(APP_STRING, cdr) returned %d\n", code);
  fprintf(stderr, "after fputs(APP_STRING, cdr) position = %ld\n", ftell(cdr));

  /* rewind */
  rewind(cdr);
  fprintf(stderr, "after rewind(cdr) position = %ld\n", ftell(cdr));
  
  /* seek to 14th psition and read 6 bytes */
  fseek(cdr, 14, SEEK_SET);
  fprintf(stderr, "after fseek(cdr, 14, SEEK_SET) position = %ld\n", ftell(cdr));
  code = fread(buffer, 1, 6, cdr);
  fprintf(stderr, "fread(buffer, 1, 6, cdr) returned %d\n", code);
  fprintf(stderr, "after fread(buffer, 1, 6, cdr) position = %ld\n", ftell(cdr));
  
  /* put those 6 bytes again to cdr */
  code = fwrite(buffer, 1, 6, cdr);
  fprintf(stderr, "fwrite(buffer, 1, 6, cdr) returned %d\n", code);
  fprintf(stderr, "after fwrite(buffer, 1, 6, cdr) position = %ld\n", ftell(cdr));

  return 0;
}
