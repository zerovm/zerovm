#include <stdio.h>
#include "zrt.h"

int main(int argc, char **argv)
{
  int i;

  /* print list of main() arguments */
  printf("list of arguments%s\n", argc == 1 ? " is empty" : ":");
  for(i = 0; i < argc; ++i)
    printf("%d. %s\n", i, argv[i]);

  return 0;
}
