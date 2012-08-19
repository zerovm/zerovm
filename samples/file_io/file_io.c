/*
 * demonstration of zrt_read(), zrt_write() and zrt_lseek()
 */
#include <stdio.h>
#include <unistd.h>
#include "zrt.h"

int main(int argc, char **argv)
{
  int i = 0;
  char buf[0x100000];

  /* write something to stdout */
  for(i = 0; i < sizeof(buf); ++i) buf[i] = i;
  write(fileno(stdout), buf, sizeof(buf));
  
  /* put stdout position to stderr(its our log) */
  fprintf(stderr,
      ":: [after write] stdin position = %ld\n"
      ":: [after write] stdout position = %ld\n"
      ":: [after write] stderr position = %ld\n",
      ftell(stdin),
      ftell(stdout),
      ftell(stderr));
  
  /* rewind stdout and read */
  rewind(stdout);
  fprintf(stderr, ":: [after rewind] stdout position = %ld\n", ftell(stdout));
  i = read(fileno(stdout), buf, sizeof(buf));
  fprintf(stderr, ":: [after read] retcode = %d\n", i);

  /* compare to written data (must be equal) */
  for(i = 0; i < sizeof(buf); ++i)
    if(buf[i] != (char)i)
      fprintf(stderr, ":: compare error in position %d[%d:%d]\n", i, (char)i, buf[i]);

  /* test seek */
  fseek(stdout, 1111, SEEK_SET);
  fprintf(stderr, ":: [after seek] stdout position = %ld\n", ftell(stdout));

  /* put stdout position to stderr(its our log) */
  fprintf(stderr, ":: [before exit] stdout position = %ld\n", ftell(stdout));
  return 0;
}
