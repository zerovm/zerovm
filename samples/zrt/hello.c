/*
 * this sample demonstrate zrt library - simple way to use libc
 * from untrusted code.
 *
 * in order to use zrt "api/zvm.h" should be included
 */
#include <stdio.h>
#include "api/zvm.h"

int main(int argc, char **argv)
{
  /* write to default device (in our case it is stdout) */
  printf("hello, world\n");

  /* write to stdout explicitly */
  fprintf(stdout, "hello, world\n");

  /* write to user log (stderr) */
  fprintf(stderr, "hello, world\n");

  return 0;
}
