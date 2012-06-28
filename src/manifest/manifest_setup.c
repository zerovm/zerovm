/*
 * construct system manifest and host manifest
 * system/channels resources counters are also here
 *
 *  Created on: Nov 30, 2011
 *      Author: d'b
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "src/utils/tools.h"
#include "src/service_runtime/include/bits/mman.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/nacl_config.h"
#include "src/service_runtime/sel_mem.h"
#include "src/service_runtime/nacl_memory_object.h"
#include "src/platform/nacl_log.h"
#include "src/service_runtime/nacl_syscall_handlers.h"

#include <src/manifest/manifest_parser.h>
#include <src/manifest/manifest_setup.h>
#include <src/manifest/mount_channel.h>

/* return md5 hash of mapped _output_ file (or NULL) */
static char* MakeEtag(struct NaClApp *nap)
{
  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  return "etag disabled";
}

/* preallocate memory area of given size. abort if fail */
static void PreallocateUserMemory(struct NaClApp *nap)
{
  uintptr_t i;
  uint32_t stump;
  uint32_t dead_space;
  struct NaClVmmapEntry *user_space;

  /* quit function if max_mem is not specified in manifest */
  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  if(nap->system_manifest->max_mem == 0) return;

  /* user memory chunk must be allocated next to the data end */
  stump = nap->system_manifest->max_mem - nap->stack_size - nap->data_end;
  i = nap->data_end;
  i = (i + NACL_MAP_PAGESIZE - 1) & ~(NACL_MAP_PAGESIZE - 1);
  nap->system_manifest->heap_ptr =
      NaClCommonSysMmapIntern(nap, (void*)i, stump, 3, 0x22, -1, 0);
  COND_ABORT(nap->system_manifest->heap_ptr != i,
      "cannot allocate specified user memory chunk");

  /*
   * free "whole chunk" block without real memory deallocation
   * the map entry we need is the last in raw
   */
  user_space = nap->mem_map.vmentry[nap->mem_map.nvalid - 1];
  assert(nap->system_manifest->heap_ptr / NACL_PAGESIZE == user_space->page_num);
  assert(nap->mem_map.is_sorted != 1);

  /* protect dead space */
  dead_space = NaClVmmapFindMaxFreeSpace(&nap->mem_map, 1) * NACL_PAGESIZE;
  i = (user_space->page_num + user_space->npages) * NACL_PAGESIZE;
  dead_space = NaClCommonSysMmapIntern(nap, (void*)i, dead_space, 0, 0x22, -1, 0);
  COND_ABORT(dead_space != i, "cannot protect dead space");

  /* force sort to remove deleted blocks */
  user_space->removed = 1;
  nap->mem_map.is_sorted = 0;
  NaClVmmapMakeSorted(&nap->mem_map);
}

/*
 * construct system_manifest object and initialize from manifest
 * todo(d'b): everythig about 'report' should be moved to HostManifestCtor()
 */
void SystemManifestCtor(struct NaClApp *nap)
{
  enum ChannelType ch;
  struct SystemManifest *policy;
  int32_t size;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  policy = nap->system_manifest;

  /* get zerovm settings from manifest */
  policy->version = GetValueByKey("Version");
  policy->log = GetValueByKey("Log");
  policy->nexe = GetValueByKey("Nexe");
  policy->blob = GetValueByKey("Blob");
  policy->nexe_etag = GetValueByKey("NexeEtag");
  GET_INT_BY_KEY(policy->nexe_max, "NexeMax");
  GET_INT_BY_KEY(policy->timeout, "Timeout");
  policy->report = fopen(GetValueByKey("Report"), "w");

  /* check mandatory manifest keys */
  COND_ABORT(nap->system_manifest->version == NULL,
      "the manifest version is not provided");
  COND_ABORT(STRCMP(nap->system_manifest->version, MANIFEST_VERSION),
      "not supported manifest version");
  size = GetFileSize(nap->system_manifest->nexe);
  COND_ABORT(size < 0, "invalid nexe");
  COND_ABORT(nap->system_manifest->nexe == NULL, "nexe name is not provided");
  if(nap->system_manifest->nexe_max) COND_ABORT(
      nap->system_manifest->nexe_max < size, "nexe file is larger then alowed");
  COND_ABORT(policy->report == NULL, "cannot open proxy report");

  /* common fields (shared with user manifest) setup */
  policy->self_size = sizeof(*policy);
  GET_INT_BY_KEY(policy->max_mem, "MemMax");
  GET_INT_BY_KEY(policy->max_syscalls, "SyscallsMax");
  policy->cnt_syscalls = 0;
  policy->syscallback = 0;

  /* read custom attributes, if absent set to empty string */
  policy->content_type[0] = '\0';
  policy->timestamp[0] = '\0';
  policy->x_object_meta_tag[0] = '\0';
  policy->user_etag[0] = '\0';
  MEMCPY(policy->content_type, GetValueByKey("ContentType"), CONTENT_TYPE_LEN);
  MEMCPY(policy->timestamp, GetValueByKey("TimeStamp"), TIMESTAMP_LEN);
  MEMCPY(policy->x_object_meta_tag, GetValueByKey("XObjectMetaTag"), X_OBJECT_META_TAG_LEN);
  MEMCPY(policy->user_etag, GetValueByKey("UserETag"), USER_TAG_LEN);

  /* prepare command line arguments for nexe */
  /* todo(d'b): replace malloc with stack allocation */
  COND_ABORT(!(policy->cmd_line = malloc(128 * sizeof(char*))),
      "cannot allocate memory for nexe command line");
  policy->cmd_line_size = 1;
  policy->cmd_line[0] = NEXE_PGM_NAME;
  policy->cmd_line[policy->cmd_line_size] =
      strtok(GetValueByKey("CommandLine"), " \t");
  while(policy->cmd_line[policy->cmd_line_size])
    policy->cmd_line[++policy->cmd_line_size] = strtok(NULL, " \t");

  /*
   * construct and mount channels
   * todo(NETWORKING): put network channels initialization here
   * todo(d'b): move the section to "mount_channel.c"
   */
  policy->channels_count = CHANNELS_COUNT; /* todo(NETWORKING): must be fixed after integration */
  policy->channels = (int64_t)malloc(policy->channels_count * sizeof(struct ChannelDesc));
  COND_ABORT((void*)policy->channels == NULL, "cannot allocate memory for channels");
  for(ch = InputChannel; ch < policy->channels_count; ++ch)
    ChannelCtor(nap, ch);

  /*
   * allocate "whole memory chunk" if specified. should be the last allocation
   * in raw because after chunk allocated there will be no free user memory
   */
  PreallocateUserMemory(nap); /* will set "heap_ptr" */
}

/* construct host_manifest and initialize from manifest */
void HostManifestCtor(struct NaClApp *nap)
{
  assert(nap != NULL);
  assert(nap->host_manifest != NULL);

  /* other report fields will be initialized at the HostManifestDtor() */
  nap->host_manifest->ret_code = OK_CODE;
}

/* deallocate memory, close files, free other resources. put everything in the place */
int SystemManifestDtor(struct NaClApp *nap)
{
  enum ChannelType ch;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /*
   * dismount and destruct channels
   * todo(NETWORKING): put network channels finalization here
   * todo(d'b): move it to "mount_channel.c"
   */
  for(ch = InputChannel; ch < nap->system_manifest->channels_count; ++ch)
    ChannelDtor(nap, ch);
  free((void*)nap->system_manifest->channels);
  nap->system_manifest->channels = (int64_t)NULL;
  nap->system_manifest->channels_count = 0;

  return OK_CODE;
}

/*
 * write report for the proxy and free used resources
 * return 0 if success, otherwise - non 0
 */
int HostManifestDtor(struct NaClApp *nap)
{
  char report[BIG_ENOUGH_SPACE];
  char ret_code[20]; /* todo(d'b):find neat solution for "magic number" */
  char user_ret_code[20]; /* todo(d'b):find neat solution for "magic number" */

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->host_manifest != NULL);

  /* check if we need to create report */
  if(nap->system_manifest->report == NULL) return OK_CODE;

  /* set report fields if asked in manifest. note: retcodes were set earlier */
  nap->host_manifest->etag = MakeEtag(nap);
  nap->host_manifest->content_type = nap->system_manifest->content_type;
  nap->host_manifest->x_object_meta_tag = nap->system_manifest->x_object_meta_tag;
  nap->host_manifest->user_ret_code = nap->exit_status;
  sprintf(ret_code, "%d", nap->host_manifest->ret_code);
  sprintf(user_ret_code, "%d", nap->host_manifest->user_ret_code);

  /* prepare text for the report */
  sprintf(report,
    "ReportRetCode        =%s\n"
    "ReportEtag           =%s\n"
    "ReportUserRetCode    =%s\n"
    "ReportContentType    =%s\n"
    "ReportXObjectMetaTag =%s\n",
    GetValueByKey("ReportRetCode") ? ret_code : "",
    GetValueByKey("ReportEtag") ? nap->host_manifest->etag : "",
    GetValueByKey("ReportUserRetCode") ? user_ret_code : "",
    GetValueByKey("ReportContentType") ? nap->host_manifest->content_type : "",
    GetValueByKey("ReportXObjectMetaTag") ? nap->host_manifest->x_object_meta_tag : "");

  /* write report and close the report file */
  fprintf(nap->system_manifest->report, "%s", report);
  fclose(nap->system_manifest->report);

  return OK_CODE;
}
