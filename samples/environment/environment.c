/*
 * demo of the new user custom attributes design
 */
#include <stdio.h>
#include "api/zrt.h"

int main(int argc, char **argv, char **envp)
{
  printf("command line parameters:\n");
  while(*argv != NULL)
    printf("\"%s\"\n", *argv++);
    
  printf("environment variables:\n");
  while(*envp != NULL)
    printf("\"%s\"\n", *envp++);

  return 0;
}
