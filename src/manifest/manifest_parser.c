/*
 * manifest_parser.c
 *
 * manifest file have a simple format: key = value
 * parser ignores white spaces
 * each line can only contain single key=value
 * parsed manifest is an array of structs "manifest_record"
 *
 * TODO: make it class with constructor/get/set methods and
 *       make the manifest read only after initialization
 * TODO: split it into 2 classes: parser and initializer
 *
 *  Created on: Nov 1, 2011
 *      Author: d'b
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*
 * todo: remove manifest_setup.h when manifest structs will
 * be moved to one place. "manifest_parser.h" and "manifest_setup.h"
 * should be used in that exact order, fix it.
 */
#include "src/manifest/manifest_parser.h"
#include "src/manifest/manifest_setup.h"


/* public function. return value from manifest by given key */
char* get_value_by_key(struct NaClApp *nap, char *key)
{
	int i;
	for(i = 0; i < nap->manifest->master_records; ++i)
	{
		if(strcmp(key, nap->manifest->master[i].key) == 0)
			return nap->manifest->master[i].value;
	}
	return NULL;
}

/* remove leading and ending spaces from the given string */
char* cut_spaces(char *a)
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
char* get_key(char *a)
{
  char *end;
  if(a == NULL) return a;

  end = strrchr(a, (int)'=');
  if(end == NULL) return NULL;	/* string does not contain '='. invalid key. */
  if(a == end) return NULL;			/* empty key is not allowed. invalid key. */
  *end	= '\0';									/* cut the string in position of the last '=' */

	return cut_spaces(a);
}

/* return string after '=' */
char* get_value(char *a)
{
  char *begin;
  if(a == NULL) return a;

  begin = strchr(a, '=');
	if(begin == NULL) return NULL;				/* string does not contain '='. invalid line. */
	if(strchr(++begin, '=')) return NULL;	/* string contain more the one '='. invalid line. */
	if(*begin == '\0') return NULL;				/* empty value is not allowed. invalid line. */

	return cut_spaces(begin);
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
int parse_manifest(const char *name, struct NaClApp *nap)
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
    nap->manifest->master[count].value = get_value(p);
    nap->manifest->master[count].key = get_key(p);
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
