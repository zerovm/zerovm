/*
 * test of user memory managed by zerovm and provided
 * trough api. MemMax field must be set in manifest
 *
 * note: so far zerovm support all 3 malloc's nacl_syscalls
 *
 * found problems:
 * 1. calloc doesn't zero allocated memory. fixed
 * 2. user space is less then specified by 1(or 2) pages and 12 bytes
 *
 * update: bget used as an untrusted side memory manager.
 * note: unlike malloc bget crashes on allocation zero bytes
 *
 */
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "include/api_tools.h"
#include "bget.h"

#undef STDLOG
#define STDLOG STDOUT
#define SHOWID ZPRINTF(STDLOG, "%s, %d\n", __func__, __LINE__)

/* initialize memory manager with the zvm given memory chunk */
void mm_init()
{
//  void *heap_start;
//  uint32_t heap_size;
//
//  /* get the pointer to heap start. its better to use nacl setbreak directly */
//  {
//    void *p;
//    p = malloc(1);
//    assert(p != NULL);
//    heap_start = p;
//    free(p);
//  }
//
//  /* get the heap size */
//  heap_size =  zvm_bulk->heap_size /* size defined in manifest */
//      - (uintptr_t)heap_start /* real start of the heap */
//      - 16*1024*1024 /* stack */
//      - 0x30000; /* to fix */

  /* initialize memory pool */
  SHOWID;
  ZPRINTF(STDLOG, "zvm_bulk->heap_ptr = %X, zvm_bulk->heap_size = %X\n",
      (uintptr_t)zvm_bulk->heap_ptr, zvm_bulk->heap_size);

  char *p = zvm_bulk->heap_ptr;
  int i = p[0];
  SHOWID;
  i = p[1];
  SHOWID;
  i = p[2];
  SHOWID;
  i = p[3];
  SHOWID;
//  p[1] = 0;
//  SHOWID;
//  p[2] = 0;
//  SHOWID;
//  p[3] = 0;
//  SHOWID;
//  p[4] = 0;
//  SHOWID;

  memset(zvm_bulk->heap_ptr, 0, zvm_bulk->heap_size);
  SHOWID;
  bpool(zvm_bulk->heap_ptr, (bufsize)zvm_bulk->heap_size);
  SHOWID;
}

/*
 * allocate memory by small portions until its possible
 * populating each chunk with particular data
 * test all chunks and deallocate them 1 by 1
 * return 0 if everything is ok
 */
#define LOOP_SIZE 0x10000
#define MAX_CHUNK_SIZE 0x100000
#define PATTERN(x) ((uintptr_t)(x) % 0x100)
static int random_test()
{
  struct {
    uint32_t size;
    void *ptr;
  } p[BIG_ENOUGH];
  int count;
  int result = 0;
  uint32_t fullness = 0;

  /* allocate and set chunks until memory end */
  for(count = 0; count < LOOP_SIZE; ++count)
  {
    p[count].size = 1 + rand() % MAX_CHUNK_SIZE;
    p[count].ptr = bget(p[count].size);
    if(p[count].ptr == NULL) break;

    fullness += p[count].size;
//    ZPRINTF(STDLOG, "p[%d].ptr = %08X, p[%d].size = %u, fullness = %u\n",
//        count, (intptr_t)p[count].ptr, count, p[count].size, fullness);
    memset(p[count].ptr, PATTERN(p[count].ptr), p[count].size);
  }

  ZPRINTF(STDLOG, "%u bytes where allocated\n", fullness);

  /* check for errors and release each chunk */
  while(--count >= 0)
  {
    register char control = PATTERN(p[count].ptr);
    register uint32_t i;
    register uint32_t limit = p[count].size;

//    ZPRINTF(STDLOG, "p[%d].ptr = %08X, p[%d].size = %u, pattern = %02X, result = %u\n",
//        count, (intptr_t)p[count].ptr, count, p[count].size, (uint8_t)control, result);

    for(i = 0; i < limit; ++i)
    {
      if(((char*)p[count].ptr)[i] == control) continue;
      ++result;
      break;
    }

    brel(p[count].ptr);
  }

  SHOWID;

  return result;
}

/* fragmentation test. ### under construction */
static int fragmentation_test()
{
  return -1;
}

int main()
{
  zvm_bulk = zvm_init();

  /*
   * new plan
   * 1. get the heap start via nacl setbreak. compare with malloc() returning
   * 2. calculate maximum available space and try to allocate it
   * 3. fill it with pattern (not 0s) and free the memory
   * 4. calloc same amount and check if it is 0s
   * 5. if the heap touches stack test for the overlap
   */

  /* initialize memory manager */
  mm_init();

  /*
   * allocate all memory by the random size chunks,
   * check for corruption in the chunks data and release
   */
  ZTEST(random_test() == 0);

  /*
   * make fragmentaion and try allocate chunk of different sizes
   */
  fragmentation_test();
//  ZTEST(fragmentation_test() == 0);

  /* count errors and exit with it */
  if(ERRCOUNT > 0)
    ZPRINTF(STDLOG, "TEST FAILED with %d errors\n", ERRCOUNT);
  else
    zput(STDLOG, "TEST SUCCEED\n\n");

  zvm_exit(ERRCOUNT);
  return 0;
}
