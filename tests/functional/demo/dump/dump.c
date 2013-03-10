/*
 * dumps whole available memory. from the trampoline start to
 * the stack end. creates the memory map.
 */
#include <limits.h>
#include "include/zvmlib.h"

/* pointer to the array of CHANNELS */
#define CHANNELS ((struct ZVMChannel*)MANIFEST->channels)

/* regions (uintptr_t) */
/* null prot: none */
#define NULL_START 0x0
#define NULL_SIZE PAGESIZE
#define NULL_END (NULL_START + NULL_SIZE)

/* trampoline prot: rx */
#define TRAMPOLINE_START NULL_END
#define TRAMPOLINE_SIZE PAGESIZE
#define TRAMPOLINE_END (TRAMPOLINE_START + TRAMPOLINE_SIZE)

/* data from user module (synthetic size) prot: rx */
#define TEXT_START TRAMPOLINE_END
#define TEXT_SIZE (TEXT_END - TEXT_START)
#define TEXT_END HEAP_START

/* user heap prot: rw */
#define HEAP_START ((uintptr_t)MANIFEST->heap_ptr)
#define HEAP_SIZE ((uintptr_t)MANIFEST->heap_size)
#define HEAP_END (HEAP_START + HEAP_SIZE)

/* unavailable area between heap and manifest (synthetic size) prot: none */
#define HOLE_START HEAP_END
#define HOLE_SIZE (HOLE_END - HOLE_START)
#define HOLE_END MANIFEST_START

/* user maninifest prot: r */
#define MANIFEST_START ((uintptr_t)CHANNELS[MANIFEST->channels_count - 1].name)
#define MANIFEST_SIZE (STACK_END - MANIFEST_START)
#define MANIFEST_END STACK_END

/* user stack prot: rw */
#define STACK_END ((uintptr_t)(-MANIFEST->stack_size))
#define STACK_SIZE ((uintptr_t)MANIFEST->stack_size)
#define STACK_START (STACK_END + STACK_SIZE)

/* units */
#define CHUNK_SIZE (0x100000 /*1mb*/)
#define FOURGIG (0x100000000UL)
#define PAGESIZE ((uintptr_t)0x10000)
#define PAINT 0xff

/* utils */
#define ROUNDDOWN_64K(a) ((a) & ~(PAGESIZE - 1))
#define ROUNDUP_64K(a) ROUNDDOWN_64K((a) + PAGESIZE - 1)
#define DUMP(m) do { FPRINTF(STDERR, "%24s = 0x%08X\n", #m, m); } while (0)

/*
 * write buffer to given "file" channel
 * if "buf" == NULL just output "size" of "PAINT" to the "file"
 */
int64_t zwrite_all(char *file, uintptr_t buf, int64_t size, int prot)
{
  /* check arguments */
  if(size < 1) return size;
  if(file == NULL) return -1;

#define OUT_LOOP(buffer) \
  do {\
    int64_t pos;\
    int64_t rest;\
    int64_t code;\
    for(pos = 0; pos < size; pos += rest)\
    {\
      rest = MIN(CHUNK_SIZE, size - pos);\
      if((code = WRITE(file, buffer, rest)) != rest)\
        return code;\
    }\
    return pos; /* return amount of written bytes */ \
  } while(0)

  /* output painting */
  if(prot == 0)
  {
    char dummy[CHUNK_SIZE];
    memset(dummy, PAINT, CHUNK_SIZE);
    OUT_LOOP(dummy);
  }
  /* output real data */
  else
    OUT_LOOP(buf + pos);
}

/* prints the user memory map (all regions) */
/* todo: print it to the string */
#define FULLDUMP(a) DUMP(a ## _START); DUMP(a ## _SIZE); DUMP(a ## _END)
static void printf_map()
{
  DUMP(MANIFEST);
  DUMP(MANIFEST->channels_count);

  FULLDUMP(NULL);
  FULLDUMP(TRAMPOLINE);
  FULLDUMP(TEXT);
  FULLDUMP(HEAP);
  FULLDUMP(HOLE);
  FULLDUMP(MANIFEST);

  /* stack have back orientation */
  DUMP(STACK_END);
  DUMP(STACK_SIZE);
  DUMP(STACK_START);
}
#undef FULLDUMP

/* todo: print it to the string */
static void print_channels()
{
  int64_t i;

  FPRINTF(STDERR, "\n");
  for(i = 0; i < MANIFEST->channels_count; ++i)
    FPRINTF(STDERR, "%02d: %12s type = %d, size = %u, %u:%u:%u:%u\n",
        i, CHANNELS[i].name, CHANNELS[i].type, CHANNELS[i].size,
        CHANNELS[i].limits[0], CHANNELS[i].limits[1],
        CHANNELS[i].limits[2], CHANNELS[i].limits[3]);
}

int main(int argc, char **argv)
{
  uint32_t code;

  /* print memory map */
  printf_map();

  /* print channels records */
  print_channels();

  FPRINTF(STDERR, "\ndump log:\n");

  /* paint NULL */
  code = zwrite_all(STDOUT, NULL_START, NULL_SIZE, 0);
  FPRINTF(STDERR, "%s written  %u bytes\n", "NULL", code);

  /* dump TRAMPOLINE */
  code = zwrite_all(STDOUT, TRAMPOLINE_START, TRAMPOLINE_SIZE, 1);
  FPRINTF(STDERR, "%s written  %u bytes\n", "TRAMPOLINE", code);

  /* dump TEXT */
  code = zwrite_all(STDOUT, TEXT_START, TEXT_SIZE, 1);
  FPRINTF(STDERR, "%s written  %u bytes\n", "TEXT", code);

  /* dump HEAP */
  code = zwrite_all(STDOUT, HEAP_START, HEAP_SIZE, 1);
  FPRINTF(STDERR, "%s written  %u bytes\n", "HEAP", code);

  /* paint HOLE */
  code = zwrite_all(STDOUT, HOLE_START, HOLE_SIZE, 0);
  FPRINTF(STDERR, "%s written  %u bytes\n", "HOLE", code);

  /* dump MANIFEST */
  code = zwrite_all(STDOUT, MANIFEST_START, MANIFEST_SIZE, 1);
  FPRINTF(STDERR, "%s written  %u bytes\n", "MANIFEST", code);

  /* dump STACK */
  code = zwrite_all(STDOUT, STACK_END, STACK_SIZE, 1);
  FPRINTF(STDERR, "%s written  %u bytes\n", "STACK", code);

  return 0;
}
