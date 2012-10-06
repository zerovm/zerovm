/*
 * this sample demonstrate issue #81 "NULL" problem
 *
 * update:
 * i believe the problem is in function definition pepper's stat.h:
 * extern int mkdir (const char *path, __mode_t __mode)
 * __THROW __nonnull ((1));
 * (see http://ohse.de/uwe/articles/gcc-attributes.html#func-nonnull)
 */

#include <sys/stat.h>
#include <sys/types.h>
#include "../include/api_tools.h"

#define BIG_ENOUGH 0x10000

/* We are substitute glibc POSIX function to our defined function */
int mkdir(const char* pathname, mode_t mode)
{
  char msg[BIG_ENOUGH];

  ZTEST(pathname == NULL);
  snprintf(msg, BIG_ENOUGH,
      "pathname = %X, sizeof pathname = %d, sizeof(uintptr_t) = %d\n",
      (uintptr_t)pathname, sizeof pathname, sizeof(uintptr_t));
  zput(STDLOG, msg);

  return 0;
}

int main(int argc, char **argv)
{
  zvm_bulk = zvm_init();
  mkdir(NULL, 0xEEEEEE);

  zvm_exit(0);
  return 0; /* not reachable */
}
