/*
 * tests EOF for the channels
 */
#include <stdio.h>
#include "api/zrt.h"

#define FILE_NAME "data"

int main(int argc, char **argv)
{
  int counter = 0;
  FILE *f = fopen(FILE_NAME, "r");
  
  /* check the file open error */
  if(f == NULL)
  {
    printf("can't open the file %s\n", FILE_NAME);
    return 1;
  }
  
  /* read the file until eof */
  for(counter = 0; !feof(f); ++counter)
    fgetc(f);

  /* report the input file bytes count */
  printf("end of file reached\n");
  printf("%d bytes were read\n", counter);

  fclose(f);
  return 0;
}
