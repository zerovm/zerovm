/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include "include/nacl_platform.h"
#include "src/service_runtime/sel_mem.h"
#include "src/platform/nacl_log.h"
#include "gtest/gtest.h"

class SelMemTest : public testing::Test {
 protected:
  virtual void SetUp();
  virtual void TearDown();
};

void SelMemTest::SetUp() {
  NaClLogModuleInit();
}

void SelMemTest::TearDown() {
  NaClLogModuleFini();
}

TEST_F(SelMemTest, AddTest) {
  struct NaClVmmap mem_map;
  int start_page_num = 32;
  int ret_code;

  ret_code = NaClVmmapCtor(&mem_map);
  EXPECT_EQ(1, ret_code);

  for (int i = 1; i <= 5; ++i) {
    ret_code = NaClVmmapAdd(&mem_map,
                            start_page_num*i,
                            i,
                            PROT_READ | PROT_EXEC,
                            (struct NaClMemObj *) NULL);
    EXPECT_EQ(1, ret_code);
    EXPECT_EQ(i, static_cast<int>(mem_map.nvalid));
    EXPECT_EQ(5, static_cast<int>(mem_map.size));
  }

  // no checks for start_page_num ..
  ret_code = NaClVmmapAdd(&mem_map,
                          start_page_num,
                          2,
                          PROT_READ,
                          (struct NaClMemObj *) NULL);
  EXPECT_EQ(6, static_cast<int>(mem_map.nvalid));
  EXPECT_EQ(10, static_cast<int>(mem_map.size));

  NaClVmmapDtor(&mem_map);
}

//TEST_F(SelMemTest, UpdateTest) {
//  struct NaClVmmap mem_map;
//
//  EXPECT_EQ(1, NaClVmmapCtor(&mem_map));
//
//  // 1st region
//  NaClVmmapUpdate(&mem_map,
//                  32,
//                  12,
//                  PROT_READ | PROT_EXEC,
//                  (struct NaClMemObj *) NULL,
//                  0);
//  EXPECT_EQ(1, static_cast<int>(mem_map.nvalid));
//
//  // no overlap
//  NaClVmmapUpdate(&mem_map,
//                  64,
//                  10,
//                  PROT_READ,
//                  (struct NaClMemObj *) NULL,
//                  0);
//  // vmmap is [32, 44], [64, 74]
//  EXPECT_EQ(2, static_cast<int>(mem_map.nvalid));
//
//  // new mapping overlaps end and start of existing mappings
//  NaClVmmapUpdate(&mem_map,
//                  42,
//                  24,
//                  PROT_READ,
//                  (struct NaClMemObj *) NULL,
//                  0);
//  // vmmap is [32, 41], [42, 66], [67, 74]
//  EXPECT_EQ(3, static_cast<int>(mem_map.nvalid));
//
//  // new mapping is in the middle of existing mapping
//  NaClVmmapUpdate(&mem_map,
//                  36,
//                  2,
//                  PROT_READ | PROT_EXEC,
//                  (struct NaClMemObj *) NULL,
//                  0);
//  // vmmap is [32, 35], [34, 36], [37, 41], [42, 66], [67, 74]
//  EXPECT_EQ(5, static_cast<int>(mem_map.nvalid));
//
//  // new mapping covers all of the existing mapping
//  NaClVmmapUpdate(&mem_map,
//                  32,
//                  6,
//                  PROT_READ | PROT_EXEC,
//                  (struct NaClMemObj *) NULL,
//                  0);
//  // vmmap is [32, 36], [37, 41], [42, 66], [67, 74]
//  EXPECT_EQ(4, static_cast<int>(mem_map.nvalid));
//
//  // remove existing mappings
//  NaClVmmapUpdate(&mem_map,
//                  40,
//                  30,
//                  PROT_READ | PROT_EXEC,
//                  (struct NaClMemObj *) NULL,
//                  1);
//  // vmmap is [32, 36], [37, 39], [71, 74]
//  EXPECT_EQ(3, static_cast<int>(mem_map.nvalid));
//
//  NaClVmmapDtor(&mem_map);
//}

TEST_F(SelMemTest, FindPageTest) {
  struct NaClVmmap mem_map;
  int ret_code;

  ret_code = NaClVmmapCtor(&mem_map);
  EXPECT_EQ(1, ret_code);

  struct NaClVmmapEntry const *entry;
  entry = NaClVmmapFindPage(&mem_map, 32);
  EXPECT_TRUE(NULL == entry);

  int start_page_num = 32;
  for (int i = 1; i <= 6; ++i) {
    ret_code = NaClVmmapAdd(&mem_map,
                            start_page_num*i,
                            2*i,
                            PROT_READ | PROT_EXEC,
                            (struct NaClMemObj *) NULL);
    EXPECT_EQ(1, ret_code);
    EXPECT_EQ(i, static_cast<int>(mem_map.nvalid));
  }
  // vmmap is [32, 34], [64, 68], [96, 102], [128, 136],
  //          [160, 170], [192, 204]

  entry = NaClVmmapFindPage(&mem_map, 16);
  EXPECT_TRUE(NULL == entry);

  entry = NaClVmmapFindPage(&mem_map, 32);
  EXPECT_TRUE(NULL != entry);

  entry = NaClVmmapFindPage(&mem_map, 34);
  EXPECT_TRUE(NULL == entry);

  entry = NaClVmmapFindPage(&mem_map, 202);
  EXPECT_TRUE(NULL != entry);

  NaClVmmapDtor(&mem_map);
}

TEST_F(SelMemTest, FindSpaceTest) {
  struct NaClVmmap mem_map;
  uintptr_t ret_code;

  ret_code = NaClVmmapCtor(&mem_map);
  EXPECT_EQ(1U, ret_code);

  // no entry
  ret_code = NaClVmmapFindSpace(&mem_map, 32);
  EXPECT_EQ(0U, ret_code);

  EXPECT_EQ(1, NaClVmmapAdd(&mem_map,
                            32,
                            10,
                            PROT_READ | PROT_EXEC,
                            (struct NaClMemObj *) NULL));
  EXPECT_EQ(1, static_cast<int>(mem_map.nvalid));
  // one entry only
  ret_code = NaClVmmapFindSpace(&mem_map, 2);
  EXPECT_EQ(0U, ret_code);

  EXPECT_EQ(1, NaClVmmapAdd(&mem_map,
                            64,
                            10,
                            PROT_READ | PROT_EXEC,
                            (struct NaClMemObj *) NULL));
  EXPECT_EQ(2U, mem_map.nvalid);

  // the space is [32, 42], [64, 74]
  ret_code = NaClVmmapFindSpace(&mem_map, 32);
  EXPECT_EQ(0U, ret_code);

  ret_code = NaClVmmapFindSpace(&mem_map, 2);
  EXPECT_EQ(62U, ret_code);

  EXPECT_EQ(1, NaClVmmapAdd(&mem_map,
                            96,
                            10,
                            PROT_READ | PROT_EXEC,
                            (struct NaClMemObj *) NULL));
  EXPECT_EQ(3U, mem_map.nvalid);

  // vmmap is [32, 42], [64, 74], [96, 106]
  // the search is from high address down
  ret_code = NaClVmmapFindSpace(&mem_map, 22);
  EXPECT_EQ(74U, ret_code);

  NaClVmmapDtor(&mem_map);
}
