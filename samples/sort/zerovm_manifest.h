/* 
 * UNDER CONSTRUCTION! MAY CHANGE.
 * zerovm manifest header. contain the structure of user side manifest.
 * user program can access this object via arguments as usual, e.g:
 * main(int argc, char **argv). argv[0] will contain the manifest if
 * strlen(argv[0]) == 0
 * it would be handy to cast argv[0] to MinorManifest:
 * struct MinorManifest *manifest = (struct MinorManifest*) *argv;
 *
 * 2011-11-20
 * d'b
 */
 
#include <stdint.h>

struct MappedFileRecord
{
  uint32_t p; /* pointer to mapped memory area */
  uint32_t size; /* size of mapped memory area */
};

struct MinorManifest
{
  char mask; /* always 0 */
  uint32_t size; /* self size */
  struct MappedFileRecord input_map_file;
  struct MappedFileRecord output_map_file;
  char fields[1]; /* text part of manifest */
};

