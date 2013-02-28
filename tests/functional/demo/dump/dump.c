/*
 * dumps whole available memory. from the trampoline start to
 * the stack end. creates the memory map.
 */
#include <limits.h>
#include "include/zvmlib.h"

/* pointer to the array of CHANNELS */
#define CHANNELS ((struct ZVMChannel*)MANIFEST->channels)

/* regions */
#define TRAMPOLINE_START ((int64_t)0x10000)
#define HEAP_END ((uintptr_t)MANIFEST->heap_ptr + MANIFEST->heap_size)
#define STACK_END ((int64_t)(-1U + 1 - MANIFEST->stack_size))

#define CHUNK_SIZE (SSIZE_MAX / 16)
#define BUMPER_SIZE (0x10000)

/* write buffer to given "file" channel */
int64_t zwrite_all(char *file, char *buf, int64_t size)
{
  int64_t pos;
  int64_t rest;

  /* check arguments */
  if(size < 1) return size;
  if(buf == NULL || file == NULL) return -1;

  for(pos = 0; pos < size; pos += rest)
  {
    rest = MIN(CHUNK_SIZE, size - pos);
    if(WRITE(file, buf + pos, rest) != rest) return -1;
  }

  return pos;
}

/* since bumper is not readable just simulate it */
void dump_bumper(char *file)
{
  char buf[BUMPER_SIZE];
  MEMSET(buf, 0xff, BUMPER_SIZE);
  zwrite_all(file, buf, BUMPER_SIZE);
}

int main(int argc, char **argv)
{
  int64_t i;

  /* memory map */
  FPRINTF(STDERR, "MEMORY MAP:\n");
  FPRINTF(STDERR, "%16s = 0x%08X\n", "trampoline start", TRAMPOLINE_START);
  FPRINTF(STDERR, "%16s = 0x%08X\n", "heap start", (uintptr_t)MANIFEST->heap_ptr);
  FPRINTF(STDERR, "%16s = 0x%08X\n", "heap end", (int64_t)HEAP_END);
  FPRINTF(STDERR, "%16s = 0x%08X\n", "stack end", STACK_END);
  FPRINTF(STDERR, "%16s = 0x%08X\n", "stack start", STACK_END + (uint64_t)MANIFEST->stack_size);

  /* put name to the bottom of the stack */
  *(uint64_t*)0xFF000000 = 0x716f74726f622764LLU;

  /* dump "small" bumper */
  dump_bumper(STDOUT);

  /* dump memory from the trampoline start to the heap end */
  i = zwrite_all(STDOUT, (void*)TRAMPOLINE_START, (int64_t)HEAP_END - TRAMPOLINE_START);

  /* dump stack */
  i = zwrite_all(STDOUT, (void*)(uintptr_t)STACK_END, (int64_t)MANIFEST->stack_size);

  /* user MANIFEST */
  FPRINTF(STDERR, "\nUSER MANIFEST:\n");
  FPRINTF(STDERR, "%16s = 0x%08X\n\n", "manifest address", (uintptr_t)MANIFEST);
  FPRINTF(STDERR, "%16s = 0x%08X\n\n", "channels address", (uintptr_t)CHANNELS);

  FPRINTF(STDERR, "%16s = 0x%08X\n", "heap start", (uintptr_t)MANIFEST->heap_ptr);
  FPRINTF(STDERR, "%16s = 0x%08X\n", "heap size", MANIFEST->heap_size);
  FPRINTF(STDERR, "%16s = 0x%08X\n", "stack size", MANIFEST->stack_size);
  FPRINTF(STDERR, "%16s = 0x%08X\n", "channels count", MANIFEST->channels_count);

  FPRINTF(STDERR, "\n");
  for(i = 0; i < MANIFEST->channels_count; ++i)
  {
    FPRINTF(STDERR, "%02d %s type = %d, size = %d, %d:%d:%d:%d\n",
        i, CHANNELS[i].name, CHANNELS[i].type, CHANNELS[i].size,
        CHANNELS[i].limits[0], CHANNELS[i].limits[1],
        CHANNELS[i].limits[2], CHANNELS[i].limits[3]);
  }

  return 0;
}
