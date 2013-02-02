/*
 * dumps whole available memory. from the trampoline start to
 * the stack end. creates the memory map.
 */
#include "include/api_tools.h"

#define TRAMPOLINE_START (uintptr_t)0x10000
#define HEAP_END ((uintptr_t)zvm_bulk->heap_ptr + zvm_bulk->heap_size)
#define STACK_END (uintptr_t)(-1U + 1 - STACK_SIZE)

int main()
{
  struct ChannelSerialized
  {
    int64_t limits[IOLimitsCount];
    int64_t size;
    uint32_t type;
    uint32_t name;
  };
  struct UserManifestSerialized
  {
    uint32_t heap_ptr;
    uint32_t heap_size;
    uint32_t stack_size;
    int32_t channels_count;
    uint32_t channels;
  };
  struct ChannelSerialized *channels;
  struct UserManifestSerialized *manifest;
  int i;

  zvm_bulk = zvm_init();
  UNREFERENCED_VAR(errcount);

  /* memory map */
  ZPRINTF(STDERR, "MEMORY MAP:\n");
  ZPRINTF(STDERR, "%-16s = 0x%08X\n", "trampoline start", TRAMPOLINE_START);
  ZPRINTF(STDERR, "%-16s = 0x%08X\n", "heap start", (uintptr_t)zvm_bulk->heap_ptr);
  ZPRINTF(STDERR, "%-16s = 0x%08X\n", "heap end", HEAP_END);
  ZPRINTF(STDERR, "%-16s = 0x%08X\n", "stack end", STACK_END);
  ZPRINTF(STDERR, "%-16s = 0x%09llX\n", "stack start", STACK_END + (uint64_t)STACK_SIZE);

  /* dump memory from the trampoline start to the heap end */
  zwrite(STDOUT, (void*)TRAMPOLINE_START, HEAP_END - TRAMPOLINE_START);

  /* dump stack */
  zwrite(STDOUT, (void*)STACK_END, STACK_SIZE);

  /* user manifest */
  manifest = (void*)*((uintptr_t*)0xFEFFFFFC);
  channels = (void*)manifest->channels;
  ZPRINTF(STDERR, "\nUSER MANIFEST:\n");
  ZPRINTF(STDERR, "%-16s = 0x%08X\n", "heap start", manifest->heap_ptr);
  ZPRINTF(STDERR, "%-16s = 0x%08X\n", "heap size", manifest->heap_size);
  ZPRINTF(STDERR, "%-16s = 0x%08X\n", "stack size", manifest->stack_size);
  ZPRINTF(STDERR, "%-16s = 0x%08X\n", "environment", (uintptr_t)zvm_bulk->envp);
  ZPRINTF(STDERR, "%-16s = 0x%08X\n", "channels count", manifest->channels_count);
  ZPRINTF(STDERR, "%-16s = 0x%08X\n\n", "channels", manifest->channels);

  for(i = 0; i < zvm_bulk->channels_count; ++i)
  {
    ZPRINTF(STDERR, "%02d %s type = %d, size = %lld, %lld:%lld:%lld:%lld\n",
        i, (char*)channels[i].name, channels[i].type, channels[i].size,
        channels[i].limits[0], channels[i].limits[1],
        channels[i].limits[2], channels[i].limits[3]);
  }

  return 0;
}
