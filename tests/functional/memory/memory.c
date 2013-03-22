/*
 * it is more zvmlib test rather than the zerovm one. however
 * it is still needed to test memory allocation
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

#define LOOP_SIZE 0x10000
#define MAX_CHUNK_SIZE 0x100000LL
#define PATTERN(x) ((uintptr_t)(x) % 0x100)

/*
 * allocate memory by small portions until its possible
 * populating each chunk with particular data
 * test all chunks and deallocate them 1 by 1
 * return 0 if everything is ok
 */
static int random_test()
{
  int count;
  int result = 0;
  uint32_t fullness = 0;
  struct
  {
    uint32_t size;
    void *ptr;
  } p[BIG_ENOUGH];

  /* allocate and set chunks until memory end */
  for(count = 0; count < LOOP_SIZE; ++count)
  {
    p[count].size = 1 + (0x123 * RAND()) % MAX_CHUNK_SIZE;
    p[count].ptr = MALLOC(p[count].size);
    if(p[count].ptr == NULL) break;

    FPRINTF(STDERR, "successful allocation: 0x%X %d\n", p[count].ptr, p[count].size);

    fullness += p[count].size;
    MEMSET(p[count].ptr, PATTERN(p[count].ptr), p[count].size);
  }

  FPRINTF(STDERR, "%u bytes where allocated\n", fullness);

  /* check for errors and release each chunk */
  while(--count >= 0)
  {
    register char control = PATTERN(p[count].ptr);
    register uint32_t i;
    register uint32_t limit = p[count].size;

    for(i = 0; i < limit; ++i)
    {
      if(((char*)p[count].ptr)[i] == control) continue;
      ++result;
      break;
    }

    FREE(p[count].ptr);
  }

  return result;
}

/* fragmentation test */
static int fragmentation_test()
{
  return 0;
}

int main()
{
  /* try to get more memory then available */
  char *p = MALLOC(268435456);
  FREE(p);

  /*
   * new plan
   * 1. get the heap start via nacl setbreak. compare with malloc() returning
   * 2. calculate maximum available space and try to allocate it
   * 3. fill it with pattern (not 0s) and free the memory
   * 4. calloc same amount and check if it is 0s
   * 5. if the heap touches stack test for the overlap
   */

  /*
   * allocate all memory by the random size chunks,
   * check for corruption in the chunks data and release
   */
  ZTEST(random_test() == 0);

  /*
   * make fragmentaion and try allocate chunk of different sizes
   * TODO: implement it
   */
  fragmentation_test();
  ZTEST(fragmentation_test() == 0);

  /*
   * todo: calloc test
   */

  /*
   * todo: realloc test
   */

  /* count errors and exit with it */
  ZREPORT;
  return 0;
}
