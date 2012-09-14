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
    "Nexe = test.nexe"\
    "NexeEtag = 12345678"\
    "CommandLine = arg1 arg2 arg3"\
    "Environment = key1, value1, key2, value2"\
    "NodeName = test_node, 1"\
    "NameServer = udp:127.0.0.1:54321"\
    \
    "== the channels section ============================================="\
    "Channel = /folder/debug, /dev/debug, 0, 0, 0, 999999, 999999"\
    "Channel = /dev/null, /dev/stdin, 0, 99999999, 99999999, 0, 0"\
    "Channel = /folder/stdout.log, /dev/stdout, 0, 0, 0, 99999999, 99999999"\
    "Channel = /folder/stderr.log, /dev/stderr, 0, 0, 0, 99999999, 99999999"\
    "Channel = tcp:2:, /dev/out/out, 0, 0, 0, 99999999, 99999999"\
    "Channel = tcp:2:, /dev/in/in, 0, 99999999, 99999999, 0, 0"\
    \
    "== the limits section ==============================================="\
    "NexeMax = 4194304"\
    "SyscallsMax = 8192"\
    "MaxMem = 100663296"\
    "Timeout = 5"\
    "Version = 09082012"

// construct manifest, setup system_manifest and host_manifest
// todo(d'b): under construction
TEST(ManifestSetupTest, FullCase)
{
  struct NaClApp stat, *nap = &stat;

  // initialize nap
  int code = 1;
  memset(nap, 0, sizeof(*nap));
  code &= DynArrayCtor(&nap->desc_tbl, 2);
//  code &= DynArrayCtor(&nap->threads, 2);
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
