/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Unit tests for code in halt_trim.c

#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error("This file is not meant for use in the TCB")
#endif

#include "gtest/gtest.h"
#include "include/nacl_macros.h"
#include "src/platform/nacl_log.h"
#include "src/validator/x86/halt_trim.h"
#include "src/validator/x86/ncinstbuffer.h"

// Copied from halt_trim.c
static const NaClMemorySize kMinHaltKeepLength = MAX_INST_LENGTH + 1;

namespace {

// Test harness for routines in halt_trim.c.
class HaltTrimTests : public ::testing::Test {
 protected:
  HaltTrimTests() {}
};

// Show that if the file ends with less than the minimum number of halts,
// no trimming occurs.
TEST_F(HaltTrimTests, TrimSmallHaltSegment) {
  uint8_t small_test[] = {
    0xf4, 0xf4, 0xf4, 0xf4
  };
  NaClMemorySize size;
  NaClPcAddress vlimit;
  NaClMemorySize small_test_size = NACL_ARRAY_SIZE(small_test);
  size = small_test_size;
  vlimit = small_test_size;
  NCHaltTrimSegment(small_test, 0, 16, &size, &vlimit);
  EXPECT_EQ(small_test_size, size);
  EXPECT_EQ(small_test_size, vlimit);
}

// Show that we trim to the nearest 32 byte boundary if there
// are a lot of halts.
TEST_F(HaltTrimTests, TrimManyHaltsTo32Boundary) {
  NaClPcAddress vbase;
  NaClMemorySize size;
  NaClMemorySize new_size;
  NaClPcAddress vlimit;
  uint8_t large_test[] = {
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
  };
  NaClMemorySize large_test_size = NACL_ARRAY_SIZE(large_test);
  for (vbase = 0; vbase < kMinHaltKeepLength; ++vbase) {
    size = large_test_size - vbase;
    vlimit = large_test_size;
    new_size = ((NaClMemorySize) 32) - vbase;
    NCHaltTrimSegment(large_test, vbase, 32, &size, &vlimit);
    EXPECT_EQ(new_size, size);
    EXPECT_EQ((NaClPcAddress) 32, vlimit);
  }
  for (vbase = kMinHaltKeepLength; vbase < (kMinHaltKeepLength + 32); ++vbase) {
    size = large_test_size - vbase;
    vlimit = large_test_size;
    new_size = ((NaClMemorySize) 64) - vbase;
    NCHaltTrimSegment(large_test, vbase, 32, &size, &vlimit);
    EXPECT_EQ(new_size, size);
    EXPECT_EQ((NaClPcAddress) 64, vlimit);
  }
  size = large_test_size - vbase;
  vlimit = large_test_size;
  NCHaltTrimSegment(large_test, vbase, 32, &size, &vlimit);
  EXPECT_EQ(vbase, size);
  EXPECT_EQ((NaClPcAddress) 96, vlimit);
}

// Show that if rounding to the nearest block alignment is too big,
// we don't change the size.
TEST_F(HaltTrimTests, TrimFailsIfBlockAlignToBig) {
  NaClMemorySize size;
  NaClPcAddress vlimit;
  uint8_t large_test[40] = {
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
  };
  size = 40;
  vlimit = 40;
  NCHaltTrimSegment(large_test, 0, 32, &size, &vlimit);
  EXPECT_EQ((NaClMemorySize) 32, size);
  EXPECT_EQ((NaClPcAddress) 32, vlimit);
  size = 20;
  vlimit = 20;
  NCHaltTrimSegment(large_test, 30, 32, &size, &vlimit);
  EXPECT_EQ((NaClMemorySize) 20, size);
  EXPECT_EQ((NaClPcAddress) 20, vlimit);
}

}  // anonymous namespace

int main(int argc, char *argv[]) {
  NaClLogModuleInit();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
