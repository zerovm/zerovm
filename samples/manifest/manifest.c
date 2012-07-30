/*
 * demo of the new manifest design. only data available for
 * the untrusted side can be shown
 */
#include <stdio.h>
#include "api/zrt.h"

int main(int argc, char **argv, char **envp)
{
  /* CommandLine */
  printf("command line parameters:\n");
  while(*argv != NULL)
    printf("\"%s\"\n", *argv++);
    
  /* Environment */
  printf("environment variables:\n");
  while(*envp != NULL)
    printf("\"%s\"\n", *envp++);
    
  /* ZeroVM API part */
  /* channels */
  
  /* memory */
  
  /* */

  return 0;
}
