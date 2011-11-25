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

#include <stdio.h>
#include <string.h>
#include "trusted/service_runtime/sel_ldr.h"
#include "trusted/service_runtime/manifest_parse.h"

/* public function. return value from manifest by given key */
char * GetValueByKey(struct NaClApp *nap, char *key)
{
	int i;
	for(i = 0; i < nap->manifest_size; ++i)
	{
		if(strcmp(key, nap->manifest[i].key) == 0)
			return nap->manifest[i].value;
	}
	return NULL;
}

/* remove leading and ending spaces from the given string */
char * CutSpaces(char *a)
{
	char *result = a;
	char *p;
	int len;

	result += strspn(a, SPACES);      // set pointer to 1st non space symbol
	len = strlen(result);
	if(len == 0) return NULL;    			// check if string is not empty
	p = result + len - 1; 						// set pointer to the last symbol of the string
	while(*p == ' ' || *p == '\t')    // skip spaces from the endof string (filling '\0')
		*p-- = '\0';

	return result;
}

/* return string until '=' */
char * GetKey(char *a)
{
	char *end = strrchr(a, (int)'=');
	if(end == NULL) return NULL;		// string does not contain '='. invalid key.
	if(a == end) return NULL;				// empty key is not allowed. invalid key.
	*end	= '\0';										// cut the string in position of the last '='

	return CutSpaces(a);
}

/* return string after '=' */
char * GetValue(char *a)
{
	char *begin = strchr(a, '=');
	if(begin == NULL) return NULL;				// string does not contain '='. invalid line.
	if(strchr(++begin, '=')) return NULL;	// string contain more the one '='. invalid line.
	if(*begin == '\0') return NULL;				// empty value is not allowed. invalid line.

	return CutSpaces(begin);
}

int ParseManifest(char *name, struct NaClApp *nap)
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
