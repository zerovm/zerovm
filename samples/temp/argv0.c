#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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
  uint32_t size; /* self size = sizeof(input_map_file) + sizeof(output_map_file) + sizeof(fields) */
  struct MappedFileRecord input_map_file;
  struct MappedFileRecord output_map_file;
  char fields[0]; /* manifest for nexe itself */
};
/* end of inclusion */
/*---------------------------- CUT --------------------------*/

//#define DEBUG

#ifdef DEBUG
#define SHOWID printf("%s() -- %d\n", __func__, __LINE__)
#else
#define SHOWID
#endif

int main(int argc, char **argv)
{ 
  char *p;
  unsigned i;
  
 // check if in argv[0] we got manifest structure
 struct MinorManifest *m = (struct MinorManifest *)argv[0];
 if(m->mask)
 {
  printf("manifest structure wasn't passed; argv[0] = %s\n", argv[0]);
  return 1;
 }
 
 /* show manifest structure */
 printf("mask = [%c]\n", m->mask);
 printf("size = %u\n", m->size);
 printf("input_map pointer = 0x%X\n", m->input_map_file.p);
 printf("input_map size = %u\n", m->input_map_file.size);
 printf("output_map pointer = 0x%X\n", m->output_map_file.p);
 printf("output_map size = %u\n", m->output_map_file.size);
 printf("size of string part of manifest: [%d]\n", strlen(m->fields)); 
 printf("string part of manifest:\n%s\n-----------------\n\n", m->fields); 
 
  SHOWID;
 /* try to read input mapped file if provided */
 if(m->input_map_file.p)
 {
  uint32_t *p = (uint32_t *)(m->input_map_file.p);
  /* just show the 1st 32-bit integer. in this test uint32 array given as input */
  printf("the 1st uint32 integer = %x\n", *p);
 }
  SHOWID;
 
 /* try to write to output mapped file */
 printf("try to write to output mapped file\n");
 p = (char*)m->output_map_file.p; SHOWID;
 for(i = 0; i < m->output_map_file.size; ++i)
 {
  p[i] = i;
 }
 printf("DONE\n");
   
   /*
 printf("try to write beyond the output mapped file end\n");
 p[m->output_map_file.size] = 'z'; SHOWID; 
 printf("DONE\n");
 */
 
  SHOWID;
 /* ..and read from it */
 printf("..and read from it\n");
 p = (char*)m->output_map_file.p; SHOWID;
 for(i = 0; i < m->output_map_file.size; ++i)
 {
  p[i] |= p[i];
 }
 printf("DONE\n");

/*
 printf("try to read beyond the output mapped file end\n");
 printf("here we are: %c\n", p[m->output_map_file.size]);
 printf("DONE\n");
 */
 
  SHOWID;
 return 13;
}

























