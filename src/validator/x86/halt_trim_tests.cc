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
  NaClMemorySize small_test_size = NACL_ARRAY_SIZE(small_test);

  EXPECT_EQ(small_test_size,
            NCHaltTrimSize(small_test, small_test_size, 16));
}

// Show that we trim to the nearest 32 byte boundary if there
// are a lot of halts.
TEST_F(HaltTrimTests, TrimManyHaltsTo32Boundary) {
  NaClMemorySize size;
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

  size = large_test_size;
  EXPECT_EQ((NaClMemorySize) 32, NCHaltTrimSize(large_test, size, 32));

  size = large_test_size - 40;
  EXPECT_EQ((NaClMemorySize) 32, NCHaltTrimSize(large_test, size, 32));
}

// Show that if rounding to the nearest block alignment is too big,
// we don't change the size.
TEST_F(HaltTrimTests, TrimFailsIfBlockAlignToBig) {
  uint8_t large_test[40] = {
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
    0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4, 0xf4,
  };

  EXPECT_EQ((NaClMemorySize) 32,  NCHaltTrimSize(large_test, 40, 32));
  EXPECT_EQ((NaClMemorySize) 20,  NCHaltTrimSize(large_test, 20, 32));
}

}  // anonymous namespace

int main(int argc, char *argv[]) {
  NaClLogModuleInit();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
