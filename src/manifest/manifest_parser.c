/*
 * zerovm manifest parser
 *
 * the manifest file format: key = value. parser ignores white spaces
 * each line can only contain single key=value
 *
 * the api is very simple: just call GetValuByKey() after
 * manifest is constructed
 *
 *  Created on: Nov 1, 2011
 *      Author: d'b
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "src/utils/tools.h"
#include "api/zvm.h" /* error codes */
#include "src/service_runtime/include/sys/errno.h"
#include "include/nacl_base.h"
#include "src/service_runtime/nacl_config.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/manifest/manifest_parser.h"

#include "src/manifest/manifest_keywords.h" /* to check keywords sync */

static int32_t mft_count; /* number of found keys */
static char *mft_data; /* area to hold manifest text */
static struct
{
  char *key;
  char *value;
} *mft_ptr; /* array of pointers to keys/values */

/* public function. return value from manifest by given key */
char* GetValueByKey(char *key)
{
	int i;
	if(key == NULL) return NULL;

	for(i = 0; i < mft_count; ++i)
	{
		if(strcmp(key, mft_ptr[i].key) == 0)
			return mft_ptr[i].value;
	}
	return NULL;
}

/* remove leading and ending spaces from the given string */
static char* cut_spaces(char *a)
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
static char* get_key(char *a)
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
static char* get_value(char *a)
{
  char *begin;
  if(a == NULL) return a;

  begin = strchr(a, '=');
	if(begin == NULL) return NULL;				/* string does not contain '='. invalid line. */
	if(strchr(++begin, '=')) return NULL;	/* string contain more the one '='. invalid line. */
	if(*begin == '\0') return NULL;				/* empty value is not allowed. invalid line. */

	return cut_spaces(begin);
}

/* parse manifest from memory. return 0 if success */
static int ParseManifest()
{
	char *str = mft_data;
	char *p;

	assert(mft_count == 0);
	assert(mft_data != 0);
	assert(mft_ptr != 0);

  /* warning: the order of extracting pair key/value does matter */
  p = strtok(str, EOL);
  while(p)
  {
    mft_ptr[mft_count].value = get_value(p);
    mft_ptr[mft_count].key = get_key(p);
  	if (mft_ptr[mft_count].key && mft_ptr[mft_count].value) ++mft_count;
		p = strtok(NULL, EOL);
  }

  /* return error if no recods were found */
  return mft_count ? 0 : ERR_CODE;
}

/*
 * parse given file name as zerovm' manifest
 * return 0 if successful, otherwise - negative error code
 */
int ManifestCtor(const char *name)
{
  int64_t mft_size;
  int retcode;
  FILE *mft = fopen(name, "r");

  /* check if manifest keywords are syncronized */
  char *keywords[] = MANIFEST_KEYWORDS;
  assert(strcmp(keywords[TheEnd], "TheEnd") == 0);
//  assert(sizeof(keywords) / sizeof(*keywords) == TheEnd + 1);

  /* get file size */
  COND_ABORT(mft == NULL, "cannot open manifest file");
  mft_size = GetFileSize(name);

  /* allocate memory to hold manifest data */
  mft_data = malloc(mft_size + 1);
  mft_ptr =  malloc(mft_size * 4);
  mft_count = 0;
  COND_ABORT(mft_data == NULL || mft_ptr == NULL,
      "cannot allocate memory to hold manifest");

  /* read data from manifest into the memory */
  retcode = fread(mft_data, 1, mft_size, mft);
  COND_ABORT(retcode != mft_size, "cannot read manifest");

  /* parse manifest */
  retcode = ParseManifest();
  COND_ABORT(retcode, "cannot parse manifest");

  /* close file and return */
  fclose(mft);
  return OK_CODE;
}

/* free resources used by manifest */
void ManifestDtor()
{
  assert(mft_data != NULL);
  assert(mft_ptr != NULL);
  assert(mft_count != 0);

  free(mft_data);
  free(mft_ptr);
  mft_data = NULL;
  mft_ptr = NULL;
  mft_count = 0;
}
