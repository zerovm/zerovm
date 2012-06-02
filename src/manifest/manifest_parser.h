/*
 * manifest_parse.h
 *
 *  Created on: Nov 1, 2011
 *      Author: d'b
 */
#ifndef SERVICE_RUNTIME_MANIFEST_PARSER_H__
#define SERVICE_RUNTIME_MANIFEST_PARSER_H__ 1

#include "include/nacl_base.h"
#include "src/service_runtime/nacl_config.h"
#include "src/service_runtime/sel_ldr.h"

#define MANIFEST_VERSION "11nov2011"
#define EOL "\r\n"

#define ALL_KEYS {"version", "zerovm", "nexe", "maxnexe", "input", "maxinput", "#etag", "#content-type", "#x-timestamp",\
"#x-object-meta-tag", "input_mnfst", "output", "maxoutput", "output_mnfst", "maxmnfstline", "maxmnfstlines", "timeout",\
"kill_timeout", "?zerovm_retcode", "?zerovm_status", "?etag", "?#retcode", "?#status", "?#content-type", "?#x-object-meta-tag"}
#define USER_KEYS {"#etag", "#content-type", "#x-timestamp", "#x-object-meta-tag"}
#define ANSWER_KEYS {"?zerovm_retcode", "?zerovm_status", "?etag", "?#retcode", "?#status", "?#content-type", "?#x-object-meta-tag"}

EXTERN_C_BEGIN

extern uint64_t GetFileSize(const char *name);
/*
 * open manifest file, parse it and initialize in the given
 * NaClApp structure two fields: "manifest" and "manifest_size"
 */
int parse_manifest(const char *name, struct NaClApp *nap);

/*
 * get value by key from the manifest. if not found - NULL
 */
char* get_value_by_key(struct NaClApp *nap, char *key);

/*
 * helper procedures. were put here for unit test
 */
char* cut_spaces(char *a);
char* get_key(char *a);
char* get_value(char *a);

EXTERN_C_END

#endif
