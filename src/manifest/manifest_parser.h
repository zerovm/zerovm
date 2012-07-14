/*
 * manifest_parse.h
 *
 *  Created on: Nov 1, 2011
 *      Author: d'b
 */
#ifndef SERVICE_RUNTIME_MANIFEST_PARSER_H__
#define SERVICE_RUNTIME_MANIFEST_PARSER_H__ 1

#include "include/nacl_base.h"

EXTERN_C_BEGIN

#define EOL "\r\n"

/*
 * parse given file name as zerovm' manifest
 * return 0 if successful, otherwise - negative error code
 */
int ManifestCtor(const char *name);

/* free resources used by manifest */
void ManifestDtor();

/* get value by key from the manifest. NULL if not found */
char* GetValueByKey(char *key);

/*
 * return number of found values from manifest by given key
 * the values pointers will be stored into provided array
 */
int GetValuesByKey(char *key, char *values[], int capacity);

/*
 * parse given string with the given delimiter
 * return number of the tokens, populate given array with them
 */
int ParseValue(char *value, char *delimiter, char *tokens[], int capacity);

/*
 * get integer value by key from the manifest. 0 if not found
 * todo(d'b): find how to inform caller if key not found
 */
#define GET_INT_BY_KEY(var, str) \
  do {\
    char *p = GetValueByKey(str);\
    var = p == NULL ? 0 : strtoll(p, NULL, 10);\
  } while(0)

EXTERN_C_END

#endif
