#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/*
 * test for "user_log" sandbox engine
 */

/*---------------------------- CUT --------------------------*/
/* should be included from our api header file */
struct MappedFileRecord
{
  uint32_t p; /* pointer area mapped into the nexe address space */
  uint32_t size;
};

struct MinorManifest
{
  char mask; /* must be 0 for identification purposes */
  uint32_t size; /* self size */
  struct MappedFileRecord input_map_file;
  struct MappedFileRecord output_map_file;
  struct MappedFileRecord user_log;
  char fields[1]; /* manifest for nexe itself, asciiz string */
};
/* end of inclusion */
/*---------------------------- CUT --------------------------*/

//#define DEBUG

#ifdef DEBUG
#define SHOWID fprintf(stderr, "%s() -- %d\n", __func__, __LINE__); fflush(stderr)
#else
#define SHOWID
#endif


/*
 * this one would be 1st and slow version of logger
 * it always calculate string length before adding a new one
 * prerequisite: log must point to log area contain valid string (at least an empty)
 * note: given pointer will be modified. it always contain only the last msg
 */
#define LOG_MSG(log, ...) sprintf(log += strlen(log), __VA_ARGS__)

int main(int argc, char **argv)
{
  char *p;

  // check if in argv[0] we got manifest structure
  struct MinorManifest *m = (struct MinorManifest *) argv[0];
  if (m->mask)
  {
    fprintf(stderr, "manifest structure wasn't passed; argv[0] = %s\n", argv[0]);

    return 1;
  }

//  /* show manifest structure */
//  fprintf(stderr, "mask = [%c]\n", m->mask);
//  fprintf(stderr, "size = %u\n", m->size);
//  fprintf(stderr, "input_map pointer = 0x%X\n", m->input_map_file.p);
//  fprintf(stderr, "input_map size = %u\n", m->input_map_file.size);
//  fprintf(stderr, "output_map pointer = 0x%X\n", m->output_map_file.p);
//  fprintf(stderr, "output_map size = %u\n", m->output_map_file.size);
//  /* recently added */
//  fprintf(stderr, "user_log pointer = 0x%X\n", m->user_log.p);
//  fprintf(stderr, "user_log size = %u\n", m->user_log.size);
//  /* end */
//  fprintf(stderr, "size of string part of manifest: [%d]\n", strlen(m->fields));
//  fprintf(stderr, "string part of manifest:\n%s\n-----------------\n\n", m->fields);

  /* try to write to log */
  p = (char*) m->user_log.p;
//  fprintf(stderr, "try to write to log\n");

  LOG_MSG(p, "--------------------\n");
  LOG_MSG(p, "hello, i am nexe log\n");
  LOG_MSG(p, "--------------------\n");
  LOG_MSG(p, "and now i'll show some of my values\n");
  LOG_MSG(p, "sizeof(char) = %d\n", sizeof(char));
  LOG_MSG(p, "sizeof(unsigned char) = %d\n", sizeof(unsigned char));
  LOG_MSG(p, "sizeof(short) = %d\n", sizeof(short));
  LOG_MSG(p, "sizeof(unsigned short) = %d\n", sizeof(unsigned short));
  LOG_MSG(p, "sizeof(int) = %d\n", sizeof(int));
  LOG_MSG(p, "sizeof(unsigned int) = %d\n", sizeof(unsigned int));
  LOG_MSG(p, "sizeof(long) = %d\n", sizeof(long));
  LOG_MSG(p, "sizeof(unsigned long) = %d\n", sizeof(unsigned long));
  LOG_MSG(p, "sizeof(float) = %d\n", sizeof(float));
  LOG_MSG(p, "sizeof(double) = %d\n", sizeof(double));
  LOG_MSG(p, "sizeof(long double) = %d\n", sizeof(long double));
  LOG_MSG(p, "sizeof(char*) = %d\n", sizeof(char*));
  LOG_MSG(p, "sizeof(void*) = %d\n", sizeof(void*));

//  fprintf(stderr, "DONE\n");
//  fprintf(stderr, "\n\n\n%s", (char*) m->user_log.p);

  return 13;
}

























