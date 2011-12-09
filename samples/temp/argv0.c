#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "api/zvm_manifest.h"

//#define DEBUG

#ifdef DEBUG
#define SHOWID fprintf(stderr, "%s() -- %d\n", __func__, __LINE__); fflush(stderr)
#else
#define SHOWID
#endif

int main(int argc, char **argv)
{
  char *p;
  unsigned i;

  // check if in argv[0] we got manifest structure
  struct MinorManifest *m = (struct MinorManifest *) argv[0];
  if (m->mask)
  {
    fprintf(stderr, "manifest structure wasn't passed; argv[0] = %s\n", argv[0]);
    fflush(stderr);
    return 1;
  }

  /* show manifest structure */
  fprintf(stderr, "mask = [%c]\n", m->mask);
  fflush(stderr);
  fprintf(stderr, "size = %u\n", m->size);
  fflush(stderr);
  fprintf(stderr, "input_map pointer = 0x%X\n", m->input_map_file.p);
  fflush(stderr);
  fprintf(stderr, "input_map size = %u\n", m->input_map_file.size);
  fflush(stderr);
  fprintf(stderr, "output_map pointer = 0x%X\n", m->output_map_file.p);
  fflush(stderr);
  fprintf(stderr, "output_map size = %u\n", m->output_map_file.size);
  fflush(stderr);
  fprintf(stderr, "size of string part of manifest: [%d]\n", strlen(m->fields));
  fflush(stderr);
  fprintf(stderr, "string part of manifest:\n%s\n-----------------\n\n", m->fields);
  fflush(stderr);

  return 11;

  SHOWID;
  /* try to read input mapped file if provided */
  if (m->input_map_file.p)
  {
    uint32_t *p = (uint32_t *) (m->input_map_file.p);
    /* just show the 1st 32-bit integer. in this test uint32 array given as input */
    fprintf(stderr, "the 1st uint32 integer = %x\n", *p);
    fflush(stderr);
  } SHOWID;

  /* try to write to output mapped file */
  fprintf(stderr, "try to write to output mapped file\n");
  fflush(stderr);
  p = (char*) m->output_map_file.p;
  SHOWID;
  for (i = 0; i < m->output_map_file.size; ++i)
  {
    p[i] = i;
  }
  fprintf(stderr, "DONE\n");
  fflush(stderr);

  /*
   fprintf(stderr, "try to write beyond the output mapped file end\n");
   p[m->output_map_file.size] = 'z'; SHOWID;
   fprintf(stderr, "DONE\n");
   */

  /* ..and read from it */
  fprintf(stderr, "..and read from it\n");
  fflush(stderr);
  p = (char*) m->output_map_file.p; SHOWID;
  for (i = 0; i < m->output_map_file.size; ++i)
  {
    p[i] |= p[i];
  }
  fprintf(stderr, "DONE\n");
  fflush(stderr);

  /*
   fprintf(stderr, "try to read beyond the output mapped file end\n");
   fprintf(stderr, "here we are: %c\n", p[m->output_map_file.size]);
   fprintf(stderr, "DONE\n");
   */

  return 13;
}

























