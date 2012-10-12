/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// Testing NativeClient cross-platfom memory management functions
#include "include/nacl_platform.h"
#include "src/platform/nacl_log.h"
#include "src/service_runtime/sel_memory.h"

#include "gtest/gtest.h"

class SelMemoryBasic : public testing::Test {
 protected:
  virtual void SetUp();
  virtual void TearDown();
  // TODO(gregoryd) - do we need a destructor here?
};

void SelMemoryBasic::SetUp() {
  NaClLogModuleInit();
}

void SelMemoryBasic::TearDown() {
  NaClLogModuleFini();
}


TEST_F(SelMemoryBasic, AllocationTests) {
  int res = 0;
  void *p = NULL;
  int size;

  size = 0x2001;  // not power of two - should be supported

  res = NaCl_page_alloc_intern_flags(&p, size, 0);
  EXPECT_EQ(0, res);
  EXPECT_NE(static_cast<void *>(NULL), p);

  NaCl_page_free(p, size);
  p = NULL;

  // Try to allocate large memory block
  size = 256 * 1024 * 1024;  // 256M

  res = NaCl_page_alloc_intern_flags(&p, size, 0);
  EXPECT_EQ(0, res);
  EXPECT_NE(static_cast<void *>(NULL), p);

  NaCl_page_free(p, size);
}

TEST_F(SelMemoryBasic, mprotect) {
  int res = 0;
  void *p = NULL;
  int size;
  char *addr;

  size = 0x100000;

  res = NaCl_page_alloc_intern_flags(&p, size, 0);
  EXPECT_EQ(0, res);
  EXPECT_NE(static_cast<void *>(NULL), p);

  // TODO(gregoryd) - since ASSERT_DEATH is not supported for client projects,
  // we cannot use gUnit to test the protection. We might want to add some
  // internal processing (based on SEH/signals) at some stage

  res = NaCl_mprotect(p, size, PROT_READ |PROT_WRITE);
  EXPECT_EQ(0, res);
  addr = reinterpret_cast<char*>(p);
  addr[0] = '5';

  res = NaCl_mprotect(p, size, PROT_READ);
  EXPECT_EQ(0, res);
  EXPECT_EQ('5', addr[0]);

  res = NaCl_mprotect(p, size, PROT_READ|PROT_WRITE|PROT_EXEC);

  NaCl_page_free(p, size);
}
