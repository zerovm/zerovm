/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Unit tests for struct NCRemainingMemory in ncinstbuffer.{h,cc}.
 */

#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error("This file is not meant for use in the TCB")
#endif

#include <stdio.h>

#include "gtest/gtest.h"

#include "trusted/validator/x86/ncinstbuffer.h"

namespace {

/* Constant defining maximum buffer size for memory. */
const size_t kMaxBufferSize = 30;

/* Test harness for class NCRemainingMemory. */
class NCRemainingMemoryTests : public ::testing::Test {
 protected:
  void SetUp();
  NCRemainingMemoryTests();
  NCRemainingMemory _memory;        /* Memory being tested. */
  uint8_t _buffer[kMaxBufferSize];  /* Memory buffer to use. */
  const size_t _buffer_size;        /* Actually size of test data. */
  static const char* kTestData;     /* Data put into memory for testing. */
};

/* Simnple sample size for testing. */
const size_t kSampleSize = 10;

const char* NCRemainingMemoryTests::kTestData = "Test data";

NCRemainingMemoryTests::NCRemainingMemoryTests()
    : _buffer_size(strlen(kTestData)) {
  /* Fill buffer with bad test data, so that we know if bad accesses occur
   * past the end of the buffer.
   */
  for (size_t i = 0; i < kMaxBufferSize; ++i) {
    _buffer[i] = 'X';
  }
  /* Now fill in the good test data. */
  for (size_t i = 0; i < _buffer_size; ++i) {
    _buffer[i] = (uint8_t) kTestData[i];
  }
}

void NCRemainingMemoryTests::SetUp() {
  NCRemainingMemoryInit(_buffer, _buffer_size, &_memory);
}

/* Test (macro) NCRemainingMemoryGetNext, and see if it properly
 * sees the data in the memory buffer.
 */
TEST_F(NCRemainingMemoryTests, GetNextGetsData) {
  /* First verify that we get text as defined in the test data. */
  for (size_t i = 0; i < _buffer_size; ++i) {
    EXPECT_EQ(NCRemainingMemoryGetNext(&_memory), (uint8_t) kTestData[i])
        << "testing GetNext(" << i << ")";
    NCRemainingMemoryRead(&_memory);
  }
  /* Now verify that if we look at the next (i.e. peek), we get 0. */
  EXPECT_EQ(0, NCRemainingMemoryGetNext(&_memory)) << "testing GetNext(eof).";
}

/* Test function NCRemainingMemoryLookahead, and see if it properly
 * sees the data in the memory buffer.
 */
TEST_F(NCRemainingMemoryTests, LookaheadGetsData) {
  size_t chars_read = 0;
  /* Try looking ahead from each possible position in the buffer. */
  for (size_t chars_left = _buffer_size; chars_left > 0; --chars_left) {
    /* First verify that for the characters left, lookahead matches. */
    for (size_t i = 0; i < chars_left; ++i) {
      EXPECT_EQ(NCRemainingMemoryLookahead(&_memory, i),
                (uint8_t) kTestData[chars_read + i])
          << "testing Lookahed(" << i << " after " << chars_read
          << " chars read.";
    }
    /* Verify that any further lookahead returns 0. */
    EXPECT_EQ(0, NCRemainingMemoryLookahead(&_memory, chars_left));
    EXPECT_EQ(0, NCRemainingMemoryLookahead(&_memory, chars_left+1));
    EXPECT_EQ(0, NCRemainingMemoryLookahead(&_memory, chars_left+10));
    EXPECT_EQ(0, NCRemainingMemoryLookahead(&_memory, chars_left+111));
    NCRemainingMemoryRead(&_memory);
    ++chars_read;
  }
}

/* Test function NCRemainingMemoryRead, and see if it properly reads
 * data from the memory buffer.
 */
TEST_F(NCRemainingMemoryTests, ReadGetsData) {
  /* First verify that we get text as defined in the test data. */
  for (size_t i = 0; i < _buffer_size; ++i) {
    EXPECT_EQ(NCRemainingMemoryRead(&_memory), (uint8_t) kTestData[i])
        << "after reading " << i << " characters.";
  }
  /* Now verify that zero is returned for any additional reads. */
  EXPECT_EQ(0, NCRemainingMemoryRead(&_memory))
      << "after reading all characters.";
  EXPECT_EQ(0, NCRemainingMemoryRead(&_memory))
      << "after reading too far past characters";
}

/* Test if the field read_length is properly updated by the read function. */
TEST_F(NCRemainingMemoryTests, ReadLengthTests) {
  /* Verify that read count is initially zero. */
  EXPECT_EQ(0, _memory.read_length) << "should be initially zero read lenght";
  /* Verify that we update the read length while there is real data. */
  for (size_t i = 0; i < _buffer_size; ++i) {
    EXPECT_EQ(i, _memory.read_length);
    NCRemainingMemoryRead(&_memory);
  }
  /* Verify that no matter how many more reads we do, the read length
   * doesn't increase.
   */
  NCRemainingMemoryRead(&_memory);
  EXPECT_EQ(_buffer_size, (size_t)_memory.read_length);
  NCRemainingMemoryRead(&_memory);
  EXPECT_EQ(_buffer_size, (size_t)_memory.read_length);
}

/* Test if the field overflow_count is properly updated by the read function. */
TEST_F(NCRemainingMemoryTests, OverflowCountTests) {
  /* Verify that the overflow count stays zero while there is still data
   * in the buffer.
   */
  for (size_t i = 0; i < _buffer_size; ++i) {
    EXPECT_EQ(0, _memory.overflow_count);
    NCRemainingMemoryRead(&_memory);
  }
  /* Verify that any further reading causes the overflow counter to
   * be bumped.
   */
  for (size_t i = 0; i < kSampleSize; ++i) {
    EXPECT_EQ(i, _memory.overflow_count);
    NCRemainingMemoryRead(&_memory);
  }
}

/* Test if function NCRemainingMemoryReset works. */
TEST_F(NCRemainingMemoryTests, ResetDataTests) {
  /* Try reseting on possible buffer positions. */
  for (size_t i = 0; i < _buffer_size; ++i) {
    /* Try different read lengths before reseting. */
    for (size_t read_size = 0; read_size < _buffer_size + kSampleSize;
         ++read_size) {
      for (size_t k = 0; k < read_size; k++) {
        NCRemainingMemoryRead(&_memory);
      }
      NCRemainingMemoryReset(&_memory);
      EXPECT_EQ(0, _memory.read_length);
      EXPECT_EQ(0, _memory.overflow_count);
      EXPECT_EQ(NCRemainingMemoryGetNext(&_memory),
                (uint8_t) kTestData[i]);
    }
    NCRemainingMemoryRead(&_memory);
    NCRemainingMemoryAdvance(&_memory);
  }
}

/* Test if function NCRemainingMemoryAdvance works. */
TEST_F(NCRemainingMemoryTests, AdvanceDataTests) {
  /* Try advancing from various locations within the memory. */
  for (size_t i = 0; i < _buffer_size; ++i) {
    SetUp();
    for (size_t j = 0; j < i; ++j) {
      NCRemainingMemoryRead(&_memory);
    }
    NCRemainingMemoryAdvance(&_memory);
    EXPECT_EQ(0, _memory.read_length);
    EXPECT_EQ(0, _memory.overflow_count);
    EXPECT_EQ(NCRemainingMemoryGetNext(&_memory),
              (uint8_t) kTestData[i]);
  }
}

};  // anonymous namespace

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
