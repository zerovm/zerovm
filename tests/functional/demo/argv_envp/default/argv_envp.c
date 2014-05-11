/*
 * argv and envp test. almost deprecated
 */
#include "include/zvmlib.h"

int main(int argc, char **argv, char **envp)
{
  int i;

  /* command line */
  printf("argc = %d\n", argc);
  for(i = 0; argv[i] != NULL; ++i)
    printf("argv[%d] = %s\n", i, argv[i]);
  printf("argv[%d] = %s\n", i, argv[i] == NULL ? "NULL" : argv[i]);

  /* environment */
  for(i = 0; envp[i] != NULL; ++i)
    printf("envp[%d] = %s\n", i, envp[i]);
  printf("envp[%d] = %s\n", i, envp[i] == NULL ? "NULL" : envp[i]);

  return 0;
}
