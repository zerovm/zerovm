/*
 * demo of the new user custom attributes design
 */
#include "include/api_tools.h"

#undef STDLOG
#define STDLOG STDOUT

int main(int argc, char **argv, char **envp)
{
  zvm_bulk = zvm_init();

  /* show command line */
  ZPRINTF(STDLOG, "COMMAND LINE PARAMETERS:\n");
  while(*argv != NULL)
    ZPRINTF(STDLOG, "\"%s\"\n", *argv++);
    
  /* show environment */
  ZPRINTF(STDLOG, "\nENVIRONMENT CONSTANTS:\n");
  while(*envp != NULL)
    ZPRINTF(STDLOG, "\"%s\"\n", *envp++);

  return 0;
}
