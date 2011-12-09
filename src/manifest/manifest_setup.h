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

#define COND_ABORT(cond, msg) if(cond) {NaClLog(LOG_FATAL, msg);}
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

EXTERN_C_END

#endif
