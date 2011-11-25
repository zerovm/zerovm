/*
 * UNDER CONSTRUCTION! MAY CHANGE. MAY CONTAIN ERRORS.
 * ZeroVM manifest header. contain the structure of user side manifest.
 * user program can access this object via arguments as usual, e.g:
 * main(int argc, char **argv). argv[0] will contain the manifest if
 * strlen(argv[0]) == 0
 * it would be handy to cast argv[0] to Manifest:
 * struct Manifest *manifest = (struct Manifest*) *argv;
 *
 * 2011-11-20
 * d'b
 */

#include <stdint.h> /* how to get rid of it? */

struct Map
{
  uint32_t    ptr;     /* pointer to mapped memory area */
  uint32_t    size;    /* size of mapped memory area */
};

struct Manifest
{
  char        mask;    /* always 0 */
  uint32_t    size;    /* self size */
  struct Map  input;   /* read only memory */
  struct Map  output;  /* read/write memory */
  char        text[1]; /* text part of manifest */
};

/* 1 if there is manifest, otherwise - 0 */
#define isMANIFEST  !**argv

/* (char*) pointer to the input buffer */
#define INPUT_PTR   (char*)((struct Manifest*) *argv -> input.ptr)

/* input buffer size */
#define INPUT_SIZE  (struct Manifest*) *argv -> input.size

/* (char*) pointer to the input buffer */
#define OUTPUT_PTR  (char*)((struct Manifest*) *argv -> output.ptr)

/* input buffer size */
#define OUTPUT_SIZE (struct Manifest*) *argv -> output.size

/* (char*) pointer to the text of manifest */
#define MANIFEST_TEXT_PTR (struct Manifest*) *argv -> text

/* add LOG - write only memory mapped to file which (or which name) will be given to proxy in report */








