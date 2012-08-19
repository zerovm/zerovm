/*
 * demonstration of zrt_fstat() (zrt_stat() contain same routine)
 */
#include <stdio.h>
#include <sys/stat.h>
#include "zrt.h"

int main(int argc, char **argv)
{
  struct stat s;
  int i = 0;

  /* get information about stdout stream */
  printf("stdout starting position = %ld\n", ftell(stdout));
  i = fstat(fileno(stdout), &s);
  fprintf(stdout, "fstat() %s\n", i < 0 ? "failed" : "successful");
  if(i < 0) return -1;

  if (S_ISREG(s.st_mode))
    puts("stdout is a redirected disk file");
  else if (S_ISCHR(s.st_mode))
    puts("stdout is a character device");

  fprintf(stdout, "st_dev = %lld\n", s.st_dev);
  fprintf(stdout, "st_ino = %lld\n", s.st_ino);
  fprintf(stdout, "st_mode = %d\n", s.st_mode);
  fprintf(stdout, "st_nlink = %d\n", s.st_nlink);
  fprintf(stdout, "st_uid = %d\n", s.st_uid);
  fprintf(stdout, "st_gid = %d\n", s.st_gid);
  fprintf(stdout, "st_rdev = %lld\n", s.st_rdev);
  fprintf(stdout, "st_size = %lld\n", s.st_size);
  fprintf(stdout, "st_blksize = %lld\n", s.st_blksize);
  fprintf(stdout, "st_blocks = %lld\n", s.st_blocks);
  fprintf(stdout, "st_atime = %lld\n", s.st_atime);
  fprintf(stdout, "st_mtime = %lld\n", s.st_mtime);
  fprintf(stdout, "st_ctime = %lld\n\n", s.st_ctime);

  return 0;
}
