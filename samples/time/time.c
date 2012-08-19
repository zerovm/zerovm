/*
 * demonstration of zrt_gettimeofday()
 */
#include <stdio.h>
#include <time.h>
#include "zrt.h"

#include <stdint.h>
#include <unistd.h>

int main()
{
  time_t cur_time;
  struct tm *date_time;

  /* call zrt_gettimeofday() indirectly */
  printf("timestamp from manifest = %ld\nwhich means..\n",
      (long)time(&cur_time));

  /* infer and print the current time and date */
  date_time = localtime(&cur_time);
  printf("date = %d-%02d-%02d time = %02d:%02d:%02d %s\n\n",
      date_time->tm_year + 1900,
      date_time->tm_mon + 1,
      date_time->tm_mday,
      date_time->tm_hour,
      date_time->tm_min,
      date_time->tm_sec,
      date_time->tm_zone);

  return 0;
}
