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
#define STACK_SIZE   0x1000000

static char s_msg[BIG_ENOUGH];

/* We are substitute glibc POSIX function to our defined function */
int mkdir(const char* pathname, mode_t mode)
{
  ZTEST(pathname == NULL);
  snprintf(s_msg, BIG_ENOUGH,
      "pathname = %X, sizeof pathname = %d, sizeof(uintptr_t) = %d\n",
      (uintptr_t)pathname, sizeof pathname, sizeof(uintptr_t));
  zput(STDLOG, s_msg);
  return 0;
}

uint32_t check_stack(){
    snprintf(s_msg, BIG_ENOUGH,
      "user heap pointer address = 0x%x\n", (intptr_t)zvm_bulk->heap_ptr);
    zput(STDLOG, s_msg);

    snprintf(s_msg, BIG_ENOUGH,
      "user memory size = %u\n", zvm_bulk->mem_size);
    zput(STDLOG, s_msg);

    if ( zvm_bulk->heap_ptr ){
        intptr_t heap_end_address = (intptr_t)zvm_bulk->heap_ptr+zvm_bulk->mem_size;
        intptr_t stack_bound = 0xFFFFFFFF-STACK_SIZE;

        snprintf(s_msg, BIG_ENOUGH,
          "calculated heap end address= 0x%x, stack bound=0x%X\n", heap_end_address, stack_bound );
        zput(STDLOG, s_msg);

        if ( stack_bound < heap_end_address ){
            return heap_end_address - stack_bound;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
  zvm_bulk = zvm_init();

  uint32_t overlap = check_stack();
  if ( overlap != 0 ){
      snprintf(s_msg, BIG_ENOUGH,
        "heap and stack overlaps on 0x%X bytes\n", overlap );
      zput(STDLOG, s_msg);
  }

  mkdir(NULL, 0xEEEEEE);

  zvm_exit(0);
  return 0; /* not reachable */
}
