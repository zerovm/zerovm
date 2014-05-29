#include "api/zvm.h"
#define SIZE 13

/* repeat given symbol n times */
static inline void repeat(int n, char a)
{
  while(n-- > 0)
    z_pwrite(1, &a, 1, 0);
}

/* draw full line of the figure */
static void draw_line(int n, int size)
{
  char endl = '\n';
  repeat(size - n, ' ');
  repeat(2 * n - 1, 'X');
  z_pwrite(1, &endl, 1, 0);
}

void _start()
{
  int i;

  /* top + middle */
  for(i = 1; i <= SIZE; ++i)
    draw_line(i, SIZE);

  /* bottom */
  for(i = SIZE - 1; i >= 0; --i)
    draw_line(i, SIZE);

  z_exit(0);
}
