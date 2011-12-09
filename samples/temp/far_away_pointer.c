#include <stdio.h>
#include <stdint.h>

int main()
{
  char *p = NULL;

  /* pointer far ahead */
  printf("uncovered treasure = %02x\n", p[10*0x100000000LL - 1]);

  /* pointer far behind */
  printf("uncovered treasure = %02x\n", p[10*0x100000000LL - 1]);

  /* pointer inaccessably far ahead */
  printf("uncovered treasure = %02x\n", p[0x123456789abLL]);

  /* pointer inaccessably far behind */
  printf("uncovered treasure = %02x\n", p[0x123456789abLL]);
}
