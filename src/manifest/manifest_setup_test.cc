/*
 * manifest_setup_test.cc
 *
 *  Created on: Nov 30, 2011
 *      Author: d'b
 */
#include <stdio.h>
#include <stdlib.h>
#include "gtest/gtest.h"
#include "api/zvm.h"
#include "src/service_runtime/sel_ldr.h"

#include "src/service_runtime/dyn_array.h"

#include "src/manifest/manifest_parser.h"
#include "src/manifest/manifest_setup.h"

/*
 * plan:
 * 1. construct maninfest from the file (write hardcoded data)
 * 2. call SystemManifestCtor() (main work is here, channels, memory manager e.t.c.)
 * 3. call HostManifestCtor()
 * 4. try UpdateSyscallsCount()
 * 5. try UpdateIOCounter()
 * 6. call SystemManifestDtor() (check files)
 * 7. call HostManifestDtor() (check report)
 */

#define MANIFEST_FILE "killme.manifest.txt"
#define NEXE_FILE "manifest.nexe"
#define INPUT_FILE "full.input.data"
#define FAKE_DATA "fake data just to extend a file over the zero size"
#define MANIFEST_DATA \
  "=====================================================================\n"\
  "== i/o keywords\n"\
  "=====================================================================\n"\
  "Input = full.input.data\n"\
  "InputMaxGet = 1024000\n"\
  "InputMaxGetCnt = 1024\n"\
  "InputMaxPut = 1024000\n"\
  "InputMaxPutCnt = 1024\n"\
  "InputMode = 0\n"\
  "\n"\
  "Output = full.output.data\n"\
  "OutputMaxGet = 1024000\n"\
  "OutputMaxGetCnt = 1024\n"\
  "OutputMaxPut = 1024000\n"\
  "OutputMaxPutCnt = 1024\n"\
  "OutputMode = 1\n"\
  "\n"\
  "UserLog = samples/zrt/manifest/full.user.log\n"\
  "UserLogMaxGet = 0\n"\
  "UserLogMaxGetCnt = 0\n"\
  "UserLogMaxPut = 65536\n"\
  "UserLogMaxPutCnt = 6554\n"\
  "UserLogMaxMode = 0\n"\
  "\n"\
  "NetInput = some_url\n"\
  "NetInputMaxGet = 1024000\n"\
  "NetInputMaxGetCnt = 1024\n"\
  "NetInputMaxPut = 1024000\n"\
  "NetInputMaxPutCnt = 1024\n"\
  "NetInputMode = 113\n"\
  "\n"\
  "NetOutput = some_another_url\n"\
  "NetOutputMaxGet = 1024000\n"\
  "NetOutputMaxGetCnt = 1024\n"\
  "NetOutputMaxPut = 1024000\n"\
  "NetOutputMaxPutCnt = 1024\n"\
  "NetOutputMode = 113\n"\
  "\n"\
  "=====================================================================\n"\
  "== user side keywords\n"\
  "=====================================================================\n"\
  "ContentType = type_name\n"\
  "TimeStamp = 1337012520\n"\
  "XObjectMetaTag = the tags list will be given to user \"as is\"\n"\
  "UserETag = reserved\n"\
  "\n"\
  "=====================================================================\n"\
  "== report request keywords\n"\
  "=====================================================================\n"\
  "ReportRetCode = bump\n"\
  "ReportEtag = bump\n"\
  "ReportUserRetCode = bump\n"\
  "ReportContentType = bump\n"\
  "ReportXObjectMetaTag = bump\n"\
  "\n"\
  "=====================================================================\n"\
  "== zerovm control keywords\n"\
  "=====================================================================\n"\
  "Version = 11062012\n"\
  "Log = full.zerovm.log\n"\
  "Report = samples/zrt/manifest/manifest.report.log\n"\
  "Nexe = manifest.nexe\n"\
  "NexeMax = 10000000\n"\
  "NexeEtag = 123456789\n"\
  "Timeout = 10\n"\
  "MemMax = 67108864\n"\
  "SyscallsMax = 32768\n"\
  "CommandLine = command line arguments for the user program. will be given as a string array\n"

// construct manifest, setup system_manifest and host_manifest
// todo(d'b): under construction
TEST(ManifestSetupTest, FullCase)
{
  struct NaClApp stat, *nap = &stat;

  // initialize nap
  int code = 1;
  memset(nap, 0, sizeof(*nap));
  code &= DynArrayCtor(&nap->desc_tbl, 2);
  code &= DynArrayCtor(&nap->threads, 2);
  code &= NaClVmmapCtor(&nap->mem_map);
  // todo(d'b): prepare memory: load nexe, allocate stack e.t.c
  if(code == 0) exit(1);

#define CREATE_FILE(name, data) \
  do {\
    FILE *f;\
    if((f = fopen(name, "w")) == NULL) return;\
    fprintf(f, data);\
    fclose(f);\
  } while(0)

  // initialize manifest
  CREATE_FILE(MANIFEST_FILE, MANIFEST_DATA);
  ManifestCtor(MANIFEST_FILE);
  EXPECT_EQ(0, ManifestCtor(MANIFEST_FILE));

  // several files from manifest must exist before setup
  CREATE_FILE(INPUT_FILE, FAKE_DATA);
  CREATE_FILE(NEXE_FILE, FAKE_DATA);

#undef CREATE_FILE

  // ### check constructed manifest validity
  EXPECT_STREQ("full.input.data", GetValueByKey((char*)"Input"));

  SystemManifestCtor(nap);

  // check constructed channels, and other fields one by one
}

//void AnswerManifestPut(struct NaClApp *nap, char *report)
//note: this test will fail when report form will change
//TEST(AnswerManifestPut_test, full_case)
//{
//  struct NaClApp *nap = allocate_nap();
//  char report[1024];
//
//  nap->host_manifest->ret_code = 0;
//  nap->host_manifest->etag = (char*)"0";
//  nap->host_manifest->user_ret_code = 0;
//  nap->host_manifest->content_type = (char*)"0";
//  nap->host_manifest->x_object_meta_tag = (char*)"0";
//
//  AnswerManifestPut(nap, report);
//  EXPECT_STREQ(
//      "ReportRetCode        =0\n"
//      "ReportEtag           =0\n"
//      "ReportUserRetCode    =0\n"
//      "ReportContentType    =0\n"
//      "ReportXObjectMetaTag =0\n", report);
//
//  free_nap(nap);

  // open report file
  // compare to expected string
//}

// main. no need to change
int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
