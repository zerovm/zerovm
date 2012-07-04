/*
 * demonstration of float/int to string conversion
 */
#include <stdio.h>
#include <stdlib.h>
#include "api/zrt.h"

int main()
{
  int8_t int8_var = -1;
  uint8_t uint8_var = 1u;
  int16_t int16_var = -22;
  uint16_t uint16_var = 22u;
  int32_t int32_var = -333;
  uint32_t uint32_var = 333u;
  int64_t int64_var = -4444l;
  uint64_t uint64_var = 4444ul;
  float float_var = -3.1415;
  double double_var = 2.718;

  fprintf(stdout, "printf() set of different variables..\n");

  printf("strtol(\"-123\", NULL, 10) = %ld\n", strtol("-123", NULL, 10));
  printf("atoi(\"321\") = %d\n", atoi("321"));

  fprintf(stdout, "int8_var = %d\n", int8_var);
  fprintf(stdout, "uint8_var = %u\n", uint8_var);
  fprintf(stdout, "int16_var = %d\n", int16_var);
  fprintf(stdout, "uint16_var = %u\n", uint16_var);
  fprintf(stdout, "int32_var = %d\n", int32_var);
  fprintf(stdout, "uint32_var = %u\n", uint32_var);
  fprintf(stdout, "int64_var = %lld\n", int64_var);
  fprintf(stdout, "uint64_var = %llu\n", uint64_var);

  fprintf(stdout, "float_var = %f\n", float_var);
  fprintf(stdout, "double_var = %f\n", double_var);

  return 0;
}
