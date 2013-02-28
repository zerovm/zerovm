/*
 * test array with 32-bit elements using algorithm from "generator"
 * return 0 if data is sorted, otherwise - 1
 */
#include "include/zvmlib.h"

#define ELEMENT_SIZE sizeof(uint32_t)
#define BUFFER_SIZE 0x4000

/* return 0 if all elements delta equal to inc */
int check_data(uint32_t inc, uint32_t *buf, uint32_t size)
{
  uint32_t i;

  for(i = 1; i < size; ++i)
    if(buf[i] - buf[i - 1] != inc) return 1;
  return 0;
}

int main(int argc, char **argv)
{
  uint32_t i;
  uint32_t buf[BUFFER_SIZE];
  uint32_t seq_size = 2; /* take in account the probe elements */
  uint32_t inc;

  /* get 2 elements and calculate the increment */
  i = READ(STDIN, buf, 2 * ELEMENT_SIZE);
  inc = i != 2 * ELEMENT_SIZE ? 0 : buf[1] - buf[0];

  /* exit on error */
  if(inc == 0) return 1;

  /*
   * read and check elements one by one, making the last element
   * the beginning one (to make ring buffer)
   */
  FPRINTF(STDERR, "checking..\n");
  for(i = 0; i == 0; buf[0] = buf[i])
  {
    /* get the next portion of data */
    i = READ(STDIN, buf, ELEMENT_SIZE * BUFFER_SIZE);
    if(i < 0)
      FPRINTF(STDERR, "errno = %d\n", i);
    if(i <= 0) break;

    i /= ELEMENT_SIZE;
    seq_size += i;
    i = check_data(inc, buf, i);
  }

  i = i || -1U / seq_size != inc;
  FPRINTF(STDERR, "the data %ssorted\n", i ? "UN" : "");
  return i;
}
