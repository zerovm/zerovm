/*
 * manifest_parse.c
 *
 * manifest file have a simple format: key = value
 * parser ignores white spaces
 * each line can only contain single key=value
 * parsed manifest is an array of structs "manifest_record"
 *
 * TODO: make it class with constructor/get/set methods
 * TODO: make the manifest read only after initialization
 *
 *  Created on: Nov 1, 2011
 *      Author: d'b
 */

// changes to answer the new manifest specification provided by camuel
// now we have two manifest files 1st: input - we read it and obey. and
// 2nd: output - we return fields asked with 1st manifest
// unclear things:
//  meaning of fields passed for nexe. how should i pass it? in command line?
//  what is "etag" some md5 just to differ 1 request from another?
//  what is maxinput/maxoutput? file size? for input it doesn't matter since zvm load it whole
//  how i supposed to pass "timestamp"? always return it via time getting syscalls? btw, is that seconds?
//  exact meaning "timeout" and "killtimeout". and what measures were used? minutes?
//  as i understood keys started with '?' should be answered in outgoing manifest. i have a few question about it:
//    - what is zvm status?
//    - regarding "etag" should i just return "etag" passed in the input manifest?
//    - is there special meaning about "x-object-meta-tag" key name? values also are unclear. if zvm pass to nexe "as is"
//      its ok, otherwise i need to know exact meaning of each value, possible values e.t.c

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "include/win/mman.h"
#include "trusted/service_runtime/include/bits/mman.h"
#include "trusted/service_runtime/sel_ldr.h"
#include "trusted/service_runtime/manifest_parser.h"
#include "trusted/service_runtime/nacl_config.h"
#include "trusted/service_runtime/sel_mem.h"
#include "trusted/service_runtime/nacl_memory_object.h"
#include "shared/platform/nacl_log.h"
#include "trusted/service_runtime/zmq_syscalls.h"

/* public function. return value from manifest by given key */
char* GetValueByKey(struct NaClApp *nap, char *key)
{
	int i;
	for(i = 0; i < nap->manifest_size; ++i)
	{
		if(strcmp(key, nap->manifest[i].key) == 0)
			return nap->manifest[i].value;
	}
	return NULL;
}

/*
 * return number of found values from manifest by given key
 * values will be stored into provided array
 * note: this is temporary function. needed to solve the problem of
 * duplicate keys in manifest
 */
int GetValuesByKey(struct NaClApp *nap, char *key, char *values[], int capacity)
{
  int count = 0;
  int i;

  /* populate array with found "value" strings */
  for(i = 0; i < nap->manifest_size; ++i)
    if(strcmp(key, nap->manifest[i].key) == 0 && count < capacity)
      values[count++] = nap->manifest[i].value;

  return count;
}

/*
 * public function. set value in manifest by given key
 * return 1 if success, otherwise - 0
 */
int SetValueByKey(struct NaClApp *nap, char *key, char *value)
{
  int i;
  for(i = 0; i < nap->manifest_size; ++i)
  {
    if(strcmp(key, nap->manifest[i].key) == 0)
    {
      nap->manifest[i].value = value;
      return 1;
    }
  }
  return 0;
}

/* remove leading and ending spaces from the given string */
char* CutSpaces(char *a)
{
  char *end;
  if (a == NULL) return a;

  while (isspace(*a)) a++;  /* Trim leading space */
  if (*a == 0) return NULL; /* if string is empty */
  end = a + strlen(a) - 1;
  while (end > a && isspace(*end))
    end--; /* Trim trailing space */
  *(end + 1) = 0;           /* Write new null terminator */

  return a;
}

/* return string until '=' */
char* GetKey(char *a)
{
  char *end;
  if(a == NULL) return a;

  end = strrchr(a, (int)'=');
  if(end == NULL) return NULL;	/* string does not contain '='. invalid key. */
  if(a == end) return NULL;			/* empty key is not allowed. invalid key. */
  *end	= '\0';									/* cut the string in position of the last '=' */

	return CutSpaces(a);
}

/* return string after '=' */
char* GetValue(char *a)
{
  char *begin;
  if(a == NULL) return a;

  begin = strchr(a, '=');
	if(begin == NULL) return NULL;				/* string does not contain '='. invalid line. */
	if(strchr(++begin, '=')) return NULL;	/* string contain more the one '='. invalid line. */
	if(*begin == '\0') return NULL;				/* empty value is not allowed. invalid line. */

	return CutSpaces(begin);
}

/* show error, deallocate resources and return error code */
#define ERR(s)\
  do {\
    printf(s);\
    if(result != NULL) free(result);\
    if(str != NULL) free(str);\
    if(f != NULL) fclose(f);\
    return 0;\
  } while (0)
int ParseManifest(const char *name, struct NaClApp *nap)
{
	struct ManifestRecord *result = NULL;
	char *str = NULL;
	char *p;
	int count = 0;
	int size;
	FILE *f;

	/* open manifest in text mode */
	f = fopen(name, "r");
	if (f == NULL) ERR("cannot open manifest file\n");

	/* get manifest size */
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (size < 1) ERR("cannot get manifest file size\n");

	/* allocate memory for the keys/values */
	str = (char*) malloc(size);
	if(str == NULL) ERR("cannot allocate memory to hold manifest\n");

	/* allocate memory for the pointers by thumb rule */
	result = (struct ManifestRecord*) malloc(2 * size);
	if(result == NULL) ERR("cannot allocate memory to hold manifest pointers\n");

  /* read manifest */
  if(size != (int)fread(str, 1, size, f)) ERR("cannot read manifest file\n");

  /* warning: the order of extracting pair key/value does matter */
  p = strtok(str, EOL);
  while(p)
  {
  	result[count].value = GetValue(p);
  	result[count].key = GetKey(p);
  	if (result[count].key && result[count].value) ++count;

		p = strtok(NULL, EOL);
  }

  /* initialize given NaClApp structure */
  if (count == 0) ERR("no records found in the manifest\n");
	nap->manifest_size = count;
	nap->manifest = realloc(result, sizeof(struct ManifestRecord) * count);
	if (nap->manifest == NULL) nap->manifest = result;

	fclose(f);
	return count;
}

/*
 * using manifest struct and given keys creates string
 * which contain manifest ready to write/pass.
 *
 * note: it assumes valid manifest in nap. no extra checkings
 * note: manifest can contain duplicate keys
 * note: by now the limit to the manifest struct size is 4kb
 */
void ManifestBuilder(struct NaClApp *nap, char *keys[], int size, char *manifest)
{
  int i;
  manifest[0] = '\0'; /* in the worst case return an empty manifest */

  /* construct a new manifest */
  for (i = 0; i < size; ++i)
  {
    int j;
    char *values[MAX_MANIFEST_LEN]; //### make more strict assumption
    int count = GetValuesByKey(nap, keys[i], values, MAX_MANIFEST_LEN);

    for(j = 0; j < count; ++j)
      sprintf(manifest += strlen(manifest), "%s = %s\n", keys[i], values[j]);
  }
}

/*
 * check size of given file and map it
 * return initialized structure (or NULL)
 * we have 3 cases here:
 * - input file -- read only
 * - output file (not exist or has 0 size) -- write only
 * - output file (not 0 size) - read/write
 * update: we have only 2 cases - r/o and r/w w/o is not supported
 * note: malloc()
 */
#define MMA_ABORT(cond, msg) if(cond) {NaClLog(LOG_FATAL, msg); return 0;}
struct MappedFileRecord* CheckAndMapFile(
    struct NaClApp *nap, char *name, char *maxsize, int mode)
{
  struct MappedFileRecord *mapped = NULL;
  uint32_t limit = maxsize == NULL ? MAX_MAP_SIZE : atoi(maxsize);
  assert(limit != 0);

  /* check if enough information has been provided */
  if(name == NULL) return NULL;

  /* create map file structure */
  mapped = (struct MappedFileRecord *) malloc(sizeof(struct MappedFileRecord));
  MMA_ABORT(mapped == NULL, "input_map_file memory allocation error\n");

  /* check file size/existence and open mode */
  mapped->size = GetFileSize(name);
  switch(mode)
  {
    case NACL_ABI_PROT_READ:
      if(mapped->size == -1 || mapped->size == 0 || mapped->size > limit)
        NaClLog(LOG_FATAL, "invalid map input file or has wrong size = %d\n", mapped->size);
      break;
    case NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE:
      /* create or enlarge output file to limit */
      if(mapped->size == 0 || mapped->size == -1)
      {
        FILE *f;
        MMA_ABORT((f = fopen(name, "w")) == NULL,
            "cannot allocate file for output map\n");

        /* put any mark to the end of allocated space */
        fseek(f, limit - 1, SEEK_SET);
        fprintf(f, "\n");
        fclose(f);
        mapped->size = limit;
      }
      MMA_ABORT(mapped->size > limit, "output file exceeded its limit\n");

      break;
    default:
      NaClLog(LOG_FATAL, "not supported file open mode - %d\n", mode);
      break;
  }

  /* map and initialize file map structure */
  mapped->p = MapFile(nap, name, mode, mapped->size);
  if(mapped->p == 0)
  {
    free(mapped);
    mapped = NULL;
  }

  return mapped;
}

/*
 * make all action specified by given manifest
 * return 1 if successful, otherwise - 0
 * note: given NaClApp must contain valid manifest
 */
int MasterManifestAnalize(struct NaClApp *nap)
{
  char *name = NULL;
  char *a;

  /* check manifest version */
  a = GetValueByKey(nap, "version");
  MMA_ABORT(strcmp(a, MANIFEST_VERSION), "wrong manifest version\n");

  /* check nexe size ### should be done before nexe loaded */
  name = GetValueByKey(nap, "nexe");
  a = GetValueByKey(nap, "maxnexe");
  MMA_ABORT(a && atoi(a) < GetFileSize(name), "nexe file is greater then alowed\n");

  /* check for input data mapping request */
  nap->input_map_file = CheckAndMapFile(
      nap,
      GetValueByKey(nap, "input"),
      GetValueByKey(nap, "maxinput"),
      NACL_ABI_PROT_READ);

  /* check for output data mapping request */
  nap->output_map_file = CheckAndMapFile(
      nap,
      GetValueByKey(nap, "output"),
      GetValueByKey(nap, "maxoutput"),
      NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE);

  return 1;
}

/*
 * create string containing manifest for nexe (or NULL)
 * note: given NaClApp must contain valid manifest
 * note: given "nexe_manifest" must have capacity more then manifest to be
 * sure it have enough space to hold newly created string
 */
void NexeManifestConstruct(struct NaClApp *nap, char* nexe_manifest)
{
  char *keys[] = USER_KEYS;

  if(nap->manifest_size)
    ManifestBuilder(nap, keys, sizeof(keys)/sizeof(*keys), nexe_manifest);
}


/*
 * prepare minor manifest in special format to pass through
 * "stack mechanism" in "sel_ldr_standard.c"
 * format: [0][(uint32_t) array size][..data..] where "data"
 * known for the nexe structure. [0] - is just a mark signaling
 * that we have a special case here
 */
void BinPackManifest(struct NaClApp *nap, char *manifest, struct MinorManifest *result)
{
  /* write header to result */
  result->mask = '\0';
  result->size = sizeof(struct MinorManifest) + strlen(manifest);
  if(nap->input_map_file) result->input_map_file = *nap->input_map_file;
  if(nap->output_map_file) result->output_map_file = *nap->output_map_file;

  /* copy given buffer with ending '\0' to result */
  sprintf(result->fields, "%s", manifest);
}

/*
 * return md5 hash of mapped _output_ file (or NULL)
 */
char* MakeEtag(struct NaClApp *nap)
{
  uintptr_t addr;

  /* check if output file exists */
  if(nap->output_map_file == NULL) return NULL;

  /* calculate effective sys address from nexe address space */
  addr = NaClUserToSys(nap, nap->output_map_file->p);
  return MD5((unsigned char*)addr, nap->output_map_file->size);
}

/*
 * UNDER CONSTRUCTION
 * return string containing answer to proxy (or NULL)
 * note: given NaClApp must contain valid manifest
 * note: "report" must have enough space to hold a new string
 * "MAX_MANIFEST_LEN" would be enough since report is subset of
 * master manifest
 */
void AnswerManifestPut(struct NaClApp *nap, int ret_code, char *report)
{
  char *p = NULL;

  /* generate manifest */
  sprintf(report, "zerovm_retcode     =%d\n", 0);
  p = GetValueByKey(nap, "log");
  sprintf(report += strlen(report), "zerovm_status      =%s\n",
      p ? p : "log file name was not provided");
  p = MakeEtag(nap);
  sprintf(report += strlen(report), "etag               =%s\n",
      p ? p : "nexe didn't have output to generate md5 hash");
  sprintf(report += strlen(report), "#retcode           =%d\n", ret_code);
  sprintf(report += strlen(report), "#status            =%s\n", "ok.");
  sprintf(report += strlen(report), "#content-type      =%s\n", "application/octet-stream");
  sprintf(report += strlen(report), "#x-object-meta-tag =%s\n", "Format:Pickle");
}
