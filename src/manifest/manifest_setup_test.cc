/*
 * manifest_setup_test.cc
 *
 *  Created on: Nov 30, 2011
 *      Author: d'b
 */

#include "gtest/gtest.h"
#include "api/zvm.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/manifest/manifest_setup.h"
//#include "src/manifest/manifest_setup.c" // trick simplifies work with static (or not defined) functions

// construct valid NaClApp object (with manifest and stuff)
// exit when memory allocation error. it is not the test fail,
// it is just disability to run the test
struct NaClApp* allocate_nap(void)
{
  // reserve space for nap and manifest
  struct NaClApp *nap = (struct NaClApp*) malloc(sizeof(struct NaClApp));
  if(nap == NULL) exit(1);

  nap->manifest = (struct Manifest*) malloc(sizeof(struct Manifest));
  if(nap->manifest == NULL) exit(1);

  nap->manifest->master = NULL;
  nap->manifest->master_records = 0;
  nap->manifest->report = (struct Report*) malloc(sizeof(struct Report));
  nap->manifest->user_setup = (struct SetupList*) malloc(sizeof(struct SetupList));
  nap->manifest->system_setup = (struct SystemList*) malloc(sizeof(struct SystemList));
  if(nap->manifest->user_setup == NULL ||
      nap->manifest->system_setup == NULL ||
      nap->manifest->report == NULL) exit(1);

  return nap;
}

// deallocate memory used by nap object
void free_nap(struct NaClApp *nap)
{
  free(nap->manifest->user_setup);
  free(nap->manifest->system_setup);
  free(nap->manifest->report);
  free(nap->manifest);
  free(nap);
}

/*
 * todo: find a way to test static functions w/o #include c-source
//static void GetChannelPrefixById(enum ChannelType ch, char *prefix)
TEST(GetChannelPrefixById_test, all_cases)
{
#define EXPECT_EQ_VOID(expect, func, out, ...) \
do {\
  func(__VA_ARGS__, out);\
  EXPECT_STREQ(expect, out);\
} while(0)

  char prefix[100];
  EXPECT_EQ_VOID("Input", GetChannelPrefixById, prefix, InputChannel);
  EXPECT_EQ_VOID("Output", GetChannelPrefixById, prefix, OutputChannel);
  EXPECT_EQ_VOID("UserLog", GetChannelPrefixById, prefix, LogChannel);
  EXPECT_EQ_VOID("NetInput", GetChannelPrefixById, prefix, NetworkInputChannel);
  EXPECT_EQ_VOID("NetOutput", GetChannelPrefixById, prefix, NetworkOutputChannel);
  // any other value will abort testing function. but how to check it?
}
*/

//int32_t ConstructChannel(struct NaClApp *nap, enum ChannelType ch)
TEST(ConstructChannel_test, all_not_null_cases)
{
  struct NaClApp *nap = allocate_nap();

  EXPECT_EQ(1, ConstructChannel(nap, InputChannel)); /* construct input channel */
  EXPECT_EQ(1, ConstructChannel(nap, OutputChannel)); /* construct output channel */
  EXPECT_EQ(1, ConstructChannel(nap, LogChannel)); /* construct user log channel */
  EXPECT_EQ(1, ConstructChannel(nap, NetworkInputChannel)); /* construct network input channel */
  EXPECT_EQ(1, ConstructChannel(nap, NetworkOutputChannel)); /* construct network  output channel */

  //### fix this part of test
  // set all channels names (Input, Output,..) to some values
//  nap->manifest->master_records = 5;
//  struct MasterManifestRecord master[5] = {
//      {(char*)"Input", (char*)"dummy"},
//      {(char*)"Output", (char*)"dummy"},
//      {(char*)"UserLog", (char*)"dummy"},
//      {(char*)"NetInput", (char*)"dummy"},
//      {(char*)"NetOutput", (char*)"dummy" }
//  };
//  nap->manifest->master = master;
//
//  EXPECT_EQ(0, ConstructChannel(nap, InputChannel)); /* construct input channel */
//  EXPECT_EQ(0, ConstructChannel(nap, OutputChannel)); /* construct output channel */
//  EXPECT_EQ(0, ConstructChannel(nap, LogChannel)); /* construct user log channel */
//  EXPECT_EQ(0, ConstructChannel(nap, NetworkInputChannel)); /* construct network input channel */
//  EXPECT_EQ(0, ConstructChannel(nap, NetworkOutputChannel)); /* construct network  output channel */

  free_nap(nap);
}

//char* MakeEtag(struct NaClApp *nap)
TEST(MakeEtag_test, all_not_null_cases)
{
  struct NaClApp *nap = allocate_nap();
  char dummy[] = "dummy";

  EXPECT_EQ(NULL, MakeEtag(nap)); /* not constructed channel case */
  nap->manifest->user_setup->channels[OutputChannel].name = (intptr_t)dummy;
  EXPECT_STREQ("etag disabled", MakeEtag(nap)); /* not constructed channel case */

  free_nap(nap);
}

//void SetupReportSettings(struct NaClApp *nap)
TEST(SetupReportSettings_test, empty_cases)
{
  struct NaClApp *nap = allocate_nap();

  // how to test report creation? the only thing that can fail is
  // memory allocation and it will automatically abort program
  // todo: in a future make a death test

  free_nap(nap);
}

//void AnswerManifestPut(struct NaClApp *nap, char *report)
//note: this test will fail when report form will change
TEST(AnswerManifestPut_test, full_case)
{
  struct NaClApp *nap = allocate_nap();
  char report[1024];

  nap->manifest->report->ret_code = 0;
  nap->manifest->report->etag = (char*)"0";
  nap->manifest->report->user_ret_code = 0;
  nap->manifest->report->content_type = (char*)"0";
  nap->manifest->report->x_object_meta_tag = (char*)"0";

  AnswerManifestPut(nap, report);
  EXPECT_STREQ(
      "ReportRetCode        =0\n"
      "ReportEtag           =0\n"
      "ReportUserRetCode    =0\n"
      "ReportContentType    =0\n"
      "ReportXObjectMetaTag =0\n", report);

  free_nap(nap);
}

//void SetupUserPolicy(struct NaClApp *nap)
// todo: add sanity tests
TEST(SetupUserPolicy_test, not_initialized_case)
{
  struct NaClApp *nap = allocate_nap();
  free(nap->manifest->user_setup); // memory will be allocated by SetupUserPolicy()
  SetupUserPolicy(nap);

  /* test setup limits */
  // nap->manifest->user_setup->self_size can be changed in a future. test disabled
  EXPECT_EQ(0, nap->manifest->user_setup->max_cpu);
  EXPECT_EQ(0, nap->manifest->user_setup->max_mem);
  EXPECT_EQ(0, nap->manifest->user_setup->max_setup_calls);
  EXPECT_EQ(0, nap->manifest->user_setup->max_syscalls);

  /* test setup counters */
  EXPECT_EQ(0, nap->manifest->user_setup->cnt_cpu);
  EXPECT_EQ(0, nap->manifest->user_setup->cnt_cpu_last);
  EXPECT_EQ(0, nap->manifest->user_setup->cnt_mem);
  EXPECT_EQ(0, nap->manifest->user_setup->cnt_setup_calls);
  EXPECT_EQ(0, nap->manifest->user_setup->cnt_syscalls);
  EXPECT_EQ(0u, nap->manifest->user_setup->heap_ptr); /* will be used in future */

  /* test syscallback niitial value */
  EXPECT_EQ(0, nap->manifest->user_setup->syscallback);

  //### check if it must be "" (not NULL)
  EXPECT_STREQ("", nap->manifest->user_setup->content_type);
  EXPECT_STREQ("", nap->manifest->user_setup->timestamp);
  EXPECT_STREQ("", nap->manifest->user_setup->x_object_meta_tag);
  EXPECT_STREQ("", nap->manifest->user_setup->user_etag);

  free_nap(nap);
}

// ### fix this test
//void SetupSystemPolicy(struct NaClApp *nap)
// todo: add sanity tests
TEST(SetupSystemPolicy_test, not_initialized_case)
{
  struct NaClApp *nap = allocate_nap();
  SetupUserPolicy(nap);

//  EXPECT_STREQ(NULL, nap->manifest->system_setup->version);
//  EXPECT_STREQ(NULL, nap->manifest->system_setup->zerovm);
//  EXPECT_STREQ(NULL, nap->manifest->system_setup->log);
//  EXPECT_STREQ(NULL, nap->manifest->system_setup->report);
//  EXPECT_STREQ(NULL, nap->manifest->system_setup->nexe);
//  EXPECT_STREQ(NULL, nap->manifest->system_setup->blob);
//  EXPECT_STREQ(NULL, nap->manifest->system_setup->nexe_etag);
//  EXPECT_EQ(0, nap->manifest->system_setup->nexe_max);
//  EXPECT_EQ(0, nap->manifest->system_setup->timeout);
//  EXPECT_EQ(0, nap->manifest->system_setup->kill_timeout);

  free_nap(nap);
}

//void PreallocateUserMemory(struct NaClApp *nap) -- useless so far

// system counters acessors -- needless so far. to remove in the future

// main. no need to change
int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
