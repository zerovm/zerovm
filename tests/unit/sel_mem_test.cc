/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include "src/include/nacl_platform.h"
#include "src/service_runtime/sel_mem.h"
#include "src/service_runtime/zlog.h"
#include "gtest/gtest.h"

class SelMemTest : public testing::Test {
 protected:
  virtual void SetUp();
  virtual void TearDown();
};

void SelMemTest::SetUp() {
  ZLogCtor(LOG_DEBUG);
}

void SelMemTest::TearDown() {
  ZLogDtor();
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
