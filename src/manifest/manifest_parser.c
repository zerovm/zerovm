/*
 * manifest_parser.c
 *
 * manifest file have a simple format: key = value
 * parser ignores white spaces
 * each line can only contain single key=value
 * parsed manifest is an array of structs "manifest_record"
 *
 * TODO: make it class with constructor/get/set methods
 * TODO: make the manifest read only after initialization
 * TODO: split it to two classes: parser and initializer
 *
 *  Created on: Nov 1, 2011
 *      Author: d'b
 */

/* ### revise headers */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "src/service_runtime/include/bits/mman.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/manifest/manifest_parser.h"
#include "src/manifest/manifest_setup.h"
#include "src/service_runtime/nacl_config.h"
#include "src/service_runtime/sel_mem.h"
#include "src/service_runtime/nacl_memory_object.h"
#include "src/platform/nacl_log.h"
/**/

#include "src/manifest/mount_channel.h"

/* public function. return value from manifest by given key */
char* GetValueByKey(struct NaClApp *nap, char *key)
{
	int i;
	for(i = 0; i < nap->manifest->master_records; ++i)
	{
		if(strcmp(key, nap->manifest->master[i].key) == 0)
			return nap->manifest->master[i].value;
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
  for(i = 0; i < nap->manifest->master_records; ++i)
    if(strcmp(key, nap->manifest->master[i].key) == 0 && count < capacity)
      values[count++] = nap->manifest->master[i].value;

  return count;
}

/*
 * public function. set value in manifest by given key
 * return 1 if success, otherwise - 0
 */
int SetValueByKey(struct NaClApp *nap, char *key, char *value)
{
  int i;
  for(i = 0; i < nap->manifest->master_records; ++i)
  {
    if(strcmp(key, nap->manifest->master[i].key) == 0)
    {
      nap->manifest->master[i].value = value;
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
    end--;                  /* Trim trailing space */
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
#define FREE(a) do { if(a) { free(a); a = NULL; } } while (0)
#define ERR(s)\
  do {\
    fprintf(stderr, s);\
    FREE(str);\
    if(nap->manifest != NULL)\
    {\
      FREE(nap->manifest->master);\
      FREE(nap->manifest);\
    }\
    if(f != NULL) fclose(f);\
    return 0;\
  } while (0)
/*
 * open given manifest file. parse it. construct "Manifest" struct and its "master" part
 * return count of records found, otherwise - 0
 * note: malloc()
 */
int ParseManifest(const char *name, struct NaClApp *nap)
{
	char *str = NULL;
	char *p;
	int count = 0;
	int size;
	FILE *f = NULL;

	/* get manifest size */
  if ((size = GetFileSize(name)) == -1)
    ERR("cannot get manifest file size\n");

	/* allocate memory for the Manifest object, manifest text and pointers */
  nap->manifest = (struct Manifest*) malloc(sizeof(*nap->manifest));
	if(nap->manifest == NULL) ERR("cannot allocate memory to hold manifest object\n");
	str = (char*) malloc(size);
	if(str == NULL) ERR("cannot allocate memory to hold text of manifest\n");
	nap->manifest->master = (struct MasterManifestRecord*) malloc(4 * size);
	if(nap->manifest->master == NULL) ERR("cannot allocate memory to hold manifest pointers\n");

  /* read manifest */
	f = fopen(name, "r");
  if(size != (int)fread(str, 1, size, f)) ERR("cannot read manifest file\n");

  /* warning: the order of extracting pair key/value does matter */
  p = strtok(str, EOL);
  while(p)
  {
    nap->manifest->master[count].value = GetValue(p);
    nap->manifest->master[count].key = GetKey(p);
  	if (nap->manifest->master[count].key && nap->manifest->master[count].value) ++count;

		p = strtok(NULL, EOL);
  }

  /* initialize given NaClApp structure */
  if (count == 0) ERR("no records found in the manifest\n");
  nap->manifest->master_records = count;
	nap->manifest->master = realloc(nap->manifest->master, sizeof(struct MasterManifestRecord) * count);
	if (nap->manifest->master == NULL) ERR("manifest master records memory reallocation error\n");

	fclose(f);
	return count;
}
#undef FREE
#undef ERR
