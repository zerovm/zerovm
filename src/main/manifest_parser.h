/*
 * manifest_parse.h
 *
 * Copyright (c) 2012, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SERVICE_RUNTIME_MANIFEST_PARSER_H__
#define SERVICE_RUNTIME_MANIFEST_PARSER_H__ 1

#include <glib.h>
#include "src/main/nacl_base.h"

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
char* GetValueByKey(const char *key);

/*
 * return number of found values from manifest by given key
 * the values pointers will be stored into provided array
 */
int GetValuesByKey(const char *key, char *values[], int capacity);

/*
 * parse given string with the given delimiter
 * return number of the tokens, populate given array with them
 */
int ParseValue(char *value, const char *delimiter, char *tokens[], int capacity);

/*
 * get integer value by key from the manifest. 0 if not found
 * todo(d'b): find how to inform caller if key not found
 */
#define GET_INT_BY_KEY(var, str) \
  do {\
    char *p = GetValueByKey(str);\
    var = p == NULL ? 0 : g_ascii_strtoll(p, NULL, 10);\
  } while(0)

EXTERN_C_END

#endif
