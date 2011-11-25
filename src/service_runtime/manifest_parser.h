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

#define EOL "\r\n"
#define MAX_STR_LEN 1024
#define MAX_MANIFEST_LEN NACL_PAGESIZE
#define MANIFEST_VERSION "11nov2011"
#define BUFFER_SIZE 0x1000
#define MAX_MAP_SIZE 0x80000000u /* should i move it to "nacl_base.h"? */

#define ALL_KEYS {"version", "zerovm", "nexe", "maxnexe", "input", "maxinput", "#etag", "#content-type", "#x-timestamp",\
"#x-object-meta-tag", "input_mnfst", "output", "maxoutput", "output_mnfst", "maxmnfstline", "maxmnfstlines", "timeout",\
"kill_timeout", "?zerovm_retcode", "?zerovm_status", "?etag", "?#retcode", "?#status", "?#content-type", "?#x-object-meta-tag"}
#define USER_KEYS {"#etag", "#content-type", "#x-timestamp", "#x-object-meta-tag"}
#define ANSWER_KEYS {"?zerovm_retcode", "?zerovm_status", "?etag", "?#retcode", "?#status", "?#content-type", "?#x-object-meta-tag"}

/*
 * this structure should be extracted to standalone header
 * it will be used both by zerovm and nexe
 */
struct MinorManifest
{
  char mask; /* must be 0 for identification purposes */
  uint32_t size; /* self size = sizeof(input_map_file) + sizeof(output_map_file) + sizeof(fields) */
  struct MappedFileRecord input_map_file;
  struct MappedFileRecord output_map_file;
  char fields[1]; /* manifest for nexe itself, at least contain '\0' */
};

EXTERN_C_BEGIN
/*
 * open manifest file, parse it and initialize in the given
 * NaClApp structure two fields: "manifest" and "manifest_size"
 */
int ParseManifest(const char *name, struct NaClApp *nap);

/*
 * takes parsed manifest from given NaClApp object and
 * provided as parameter list of keys (last key must be NULL) and
 * creates string which contain manifest ready to write/pass
 * return resulting string pointer
 *
 * note: it assumes valid manifest in nap. no extra checkings
 */
void ManifestBuilder(struct NaClApp *nap, char *keys[], int size, char *result);

/*
 * make all action specified by given manifest
 * note: mem_map should be already valid
 * return 1 if successful, otherwise - 0
 */
int MasterManifestAnalize(struct NaClApp *nap);

/*
 * prepare string containing manifest for nexe
 */
void NexeManifestConstruct(struct NaClApp *nap, char* nexe_manifest);

/*
 * prepare string containing proxi requested tags
 */
void AnswerManifestPut(struct NaClApp *nap, int ret_code, char *report);

/*
 * prepare minor manifest in special format to pass through
 * "stack mechanism" in "sel_ldr_standard.c" --
 * format: [0][(uint32_t) array size][..data..] where "data"
 * known for the nexe structure. [0] - is just a mark signaling
 * that we have a special case here
 *
 * note: new()
 */
void BinPackManifest(struct NaClApp *nap, char *manifest, struct MinorManifest *result);

/*
 * get value by key from the manifest. if not found - NULL
 */
char* GetValueByKey(struct NaClApp *nap, char *key);

/*
 * public function. set value in manifest by given key
 * return 1 if success, otherwise - 0
 */
int SetValueByKey(struct NaClApp *nap, char *key, char *value);

/*
 * helper procedures. were put here for unit test
 */
char* CutSpaces(char *a);
char* GetKey(char *a);
char* GetValue(char *a);

/* declaration: md5 digest calculation */
char* MD5 (unsigned char *buf, unsigned size);

EXTERN_C_END

#endif
