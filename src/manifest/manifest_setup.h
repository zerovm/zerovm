/*
 * manifest_setup.h
 *
 *  Created on: Nov 30, 2011
 *      Author: d'b
 */

#ifndef SERVICE_RUNTIME_MANIFEST_SETUP_H__
#define SERVICE_RUNTIME_MANIFEST_SETUP_H__ 1

EXTERN_C_BEGIN

#include "api/zvm_manifest.h"

#define COND_ABORT(cond, msg) if(cond) {fprintf(stderr, "%s\n", msg); exit(1);}
#define MAX_MAP_SIZE 0x80000000u

/*
 * whole master manifest is an array made from string
 * with zeroing ends of tokens and array of pointers to tokens
 */
struct MasterManifestRecord
{
  char *key;
  char *value;
};

struct SystemList
{
  char *version; /* zerovm version */
  char *zerovm;
  char *log; /* zerovm log file name */
  char *report;  /* report to proxy file name */
  char *nexe; /* nexe file name */
  char *cmd_line; /* command line for nexe */
  char *blob; /* blob library name */
  int32_t nexe_max; /* max allowed nexe length */
  char *nexe_etag; /* digital signature. reserved for a future "short" nexe validation */
  int32_t timeout;
  int32_t kill_timeout;
};

struct Report
{
  int32_t ret_code; /* zerovm return code */
  char *etag; /* user output memory digital signature */
  int32_t user_ret_code; /* nexe return code */
  char *content_type; /* custom user attribute */
  char *x_object_meta_tag; /* custom user attribute */
};

/*
 * BIG MANIFEST
 * contain all other manifests, reports and settings for zerovm, proxy and nexe
 * set to NULL if manifest was not given to zerovm
 */
struct Manifest
{
  /* text manifest given by proxy */
  uint32_t master_records; /* amount of records in master manifest */
  struct MasterManifestRecord *master; /* array of master records */

  /* limits, file i/o and counters for user program */
  /* user hints also could be passed through this structure */
  struct SetupList *user_setup;

  /* settings for zerovm */
  struct SystemList *system_setup;

  /* report fields */
  struct Report *report;
};

/*
 * construct Report (zerovm report to proxy) part of manifest structure
 * note: malloc(). must be called only once
 */
void SetupReportSettings(struct NaClApp *nap);

/*
 * prepare string containing proxi requested tags
 */
void AnswerManifestPut(struct NaClApp *nap, char *report);

/*
 * construct SetupList (policy) part of manifest structure
 * note: malloc(). must be called only once
 */
void SetupUserPolicy(struct NaClApp *nap);

/*
 * construct SystemList (zerovm settings) part of manifest structure
 * note: malloc(). must be called only once
 */
void SetupSystemPolicy(struct NaClApp *nap);

/* declaration: md5 digest calculation */
char* MD5(unsigned char *buf, unsigned size);

/*
 * construct i/o channel. the function contain "hardcoded" manifest keywords
 * if successful return initialized, but not mounted object, otherwise - NULL
 * note: malloc()
 */
int32_t ConstructChannel(struct NaClApp *nap, enum ChannelType ch);

/*
 * preallocate memory area of given size. abort if fail
 */
void PreallocateUserMemory(struct NaClApp *nap);

EXTERN_C_END

#endif
