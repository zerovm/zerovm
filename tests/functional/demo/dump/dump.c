/*
 * dumps whole available memory. from the trampoline start to
 * the stack end. creates the memory map.
 */
#include <limits.h>
#include <sys/param.h> /* MIN/MAX */
#include "include/api_tools.h"

/* pointer to the user manifest */
#define MANIFEST ((struct UserManifest*)*((uintptr_t*)0xFEFFFFFC))

/* pointer to the array of channels */
#define CHANNELS ((struct ZVMChannel*)MANIFEST->channels)

/* regions */
#define TRAMPOLINE_START ((int64_t)0x10000)
#define HEAP_END ((uintptr_t)zvm_bulk->heap_ptr + zvm_bulk->heap_size)
#define STACK_END ((int64_t)(-1U + 1 - STACK_SIZE))

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
    if(zwrite(file, buf + pos, rest) != rest) return -1;
  }

  return pos;
}

/* since bumper is not readable just simulate it */
void dump_bumper(char *file)
{
  char buf[BUMPER_SIZE];
  memset(buf, 0xff, BUMPER_SIZE);
  zwrite_all(file, buf, BUMPER_SIZE);
}

int main()
{
  struct UserManifest *manifest = MANIFEST;
  struct ZVMChannel *channels = CHANNELS;
  int64_t i;

  zvm_bulk = zvm_init();
  UNREFERENCED_VAR(errcount);

  /* memory map */
  ZPRINTF(STDERR, "MEMORY MAP:\n");
  ZPRINTF(STDERR, "%-16s = 0x%08llX\n", "trampoline start", TRAMPOLINE_START);
  ZPRINTF(STDERR, "%-16s = 0x%08X\n", "heap start", (uintptr_t)zvm_bulk->heap_ptr);
  ZPRINTF(STDERR, "%-16s = 0x%08llX\n", "heap end", (int64_t)HEAP_END);
  ZPRINTF(STDERR, "%-16s = 0x%08llX\n", "stack end", STACK_END);
  ZPRINTF(STDERR, "%-16s = 0x%09llX\n", "stack start", STACK_END + (uint64_t)STACK_SIZE);

  /* dump "small" bumper */
  dump_bumper(STDOUT);

  /* dump memory from the trampoline start to the heap end */
  i = zwrite_all(STDOUT, (char*)TRAMPOLINE_START, (int64_t)HEAP_END - TRAMPOLINE_START);

  /* dump stack */
  i = zwrite_all(STDOUT, (char*)STACK_END, (int64_t)STACK_SIZE);

  /* user manifest */
  ZPRINTF(STDERR, "\nUSER MANIFEST:\n");
  ZPRINTF(STDERR, "%-16s = 0x%08X\n\n", "manifest address", (uintptr_t)manifest);
  ZPRINTF(STDERR, "%-16s = 0x%08X\n\n", "channels address", (uintptr_t)manifest->channels);

  ZPRINTF(STDERR, "%-16s = 0x%08X\n", "heap start", (uintptr_t)manifest->heap_ptr);
  ZPRINTF(STDERR, "%-16s = 0x%08X\n", "heap size", manifest->heap_size);
  ZPRINTF(STDERR, "%-16s = 0x%08X\n", "stack size", manifest->stack_size);
  ZPRINTF(STDERR, "%-16s = 0x%08X\n", "environment", (uintptr_t)zvm_bulk->envp);
  ZPRINTF(STDERR, "%-16s = 0x%08X\n", "channels count", manifest->channels_count);

  for(i = 0; i < zvm_bulk->channels_count; ++i)
  {
    ZPRINTF(STDERR, "%02lld %s type = %d, size = %lld, %lld:%lld:%lld:%lld\n",
        i, channels[i].name, channels[i].type, channels[i].size,
        channels[i].limits[0], channels[i].limits[1],
        channels[i].limits[2], channels[i].limits[3]);
  }

  return 0;
}
