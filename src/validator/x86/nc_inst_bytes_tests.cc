/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Unit tests for struct NCInstBytes in ncinstbuffer.{h,cc}.
 */

#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error("This file is not meant for use in the TCB")
#endif

#include <stdio.h>

#include "gtest/gtest.h"

#include "src/validator/x86/ncinstbuffer.h"

namespace {

/* Constant defining maximum buffer size for memory. */
const size_t kMaxBufferSize = 30;

/* Test harness for class NCRemainingMemory. */
class NCInstBytesTests : public ::testing::Test {
 protected:
  void SetUp();
  NCInstBytesTests();

  /* Data for testing. */
  NCInstBytes _bytes;                /* Inst bytes (read). */
  NCRemainingMemory _memory;         /* Memory being tested. */
  uint8_t _buffer[kMaxBufferSize];   /* Memory buffer to use. */
  const size_t _buffer_size;         /* Actually size of test data. */
  static const char* kTestData;      /* Data put into memory for testing. */

  /* Verify if the contents of kTestData matches the contents
   * of _bytes, up to _buffer_size; Remaining elements are zero.
   */
  void VerifyBytesMatchTestData();
};

const char* NCInstBytesTests::kTestData = "Test data";

NCInstBytesTests::NCInstBytesTests()
    : _buffer_size(strlen(kTestData)) {
  /* Fill memory buffer with bad test data, so that we know if bad accesses
   * occur past the end of the buffer.
   */
  for (size_t i = 0; i < kMaxBufferSize; ++i) {
    _buffer[i] = 'X';
  }
  /* Now fill in the good test data. */
  for (size_t i = 0; i < _buffer_size; ++i) {
    _buffer[i] = (uint8_t) kTestData[i];
  }
}

void NCInstBytesTests::VerifyBytesMatchTestData() {
  /* First show that while there is real data in the memory,
   * it has been copied to the bytes buffer.
   */
  size_t match_limit =
      (_bytes.length <= _buffer_size) ? _bytes.length : _buffer_size;
  for (size_t i = 0; i < match_limit; ++i) {
    EXPECT_EQ(_bytes.byte[i], kTestData[i])
        << "value verification of byte " << i;
  }
  /* Now show that any remaining bytes (after the memory
   * has been read) are 0.
   */
  for (size_t i = match_limit; i < _bytes.length; ++i) {
    EXPECT_EQ(0, _bytes.byte[i])
        << "zero verification of byte " << i;
  }
}

void NCInstBytesTests::SetUp() {
  NCRemainingMemoryInit(_buffer, _buffer_size, &_memory);
  NCInstBytesInitMemory(&_bytes, &_memory);
  /* Be sure to verify that we can run the tests, which require
   * a couple of extra slots for overflow.
   */
  ASSERT_LT(_buffer_size + 1, (size_t) MAX_INST_LENGTH)
      << "Set up failed due to kTestData being too long.";
}

/* Test function NCInstBytesPeek, and see if it properly sees the data in
 * the instruction buffer.
 */
TEST_F(NCInstBytesTests, PeekGetsData) {
  /* First verify that we get text as defined in the test data. */
  for (size_t i = 0; i < _buffer_size; ++i) {
    EXPECT_EQ(NCInstBytesPeek(&_bytes, i), (uint8_t) kTestData[i])
        << "peeking " << i << " bytes ahead.";
  }
  /* Now verify that if we look past the end of the data, we
   * get 0.
   */
  EXPECT_EQ(0, NCInstBytesPeek(&_bytes, _buffer_size))
      << "peeking one byte past end of buffer.";
  EXPECT_EQ(0, NCInstBytesPeek(&_bytes, _buffer_size + 100))
      << "peeking 100 bytes past end of buffer.";
  EXPECT_EQ(0, _bytes.length);
  VerifyBytesMatchTestData();
}

/* Test function NCInstBytesRead, and see if it properly read
 * data from the memory buffer.
 */
TEST_F(NCInstBytesTests, ReadGetsData) {
  /* First verify that we get text as defined in the test data. */
  for (size_t i = 0; i < _buffer_size; ++i) {
    EXPECT_EQ(NCInstBytesRead(&_bytes), (uint8_t) kTestData[i])
        << "after reading " << i << " characters";
    VerifyBytesMatchTestData();
  }
  EXPECT_EQ(_buffer_size, _bytes.length);
  VerifyBytesMatchTestData();
  /* Now verify that zero is returned for any additional reads. */
  for (size_t i = _bytes.length; i < MAX_INST_LENGTH; ++i) {
    EXPECT_EQ(0, NCInstBytesRead(&_bytes))
        << "after reading all characters";
  }
  VerifyBytesMatchTestData();
  /* Verify that we can apply a read, even if buffer overflow occurs,
   * and the only effect is that the buffer length is not increased.
   */
  EXPECT_EQ(MAX_INST_LENGTH, _bytes.length);
  EXPECT_EQ(0, NCInstBytesRead(&_bytes));
  EXPECT_EQ(MAX_INST_LENGTH, _bytes.length);
}

/* Test function NCInstBytesReadBytes, and see if it properly reads
 * data from the memory buffer.
 */
TEST_F(NCInstBytesTests, ReadBytesGetsData) {
  /* First verify that we get text as defined in the test data. */
  size_t chars_read = 0;
  size_t stride = 4;
  VerifyBytesMatchTestData();
  while (chars_read < MAX_INST_LENGTH) {
    if ((chars_read + stride) > MAX_INST_LENGTH) {
      stride = MAX_INST_LENGTH - chars_read;
    }
    NCInstBytesReadBytes(stride, &_bytes);
    chars_read += stride;
    VerifyBytesMatchTestData();
  }
  /* Verify that we can apply additional reads without overflowing
   * the buffer.
   */
  NCInstBytesReadBytes(2, &_bytes);
  EXPECT_EQ(MAX_INST_LENGTH, _bytes.length) << "Buffer overflow fix failed.";
}

/* Test function NCInstBytesReset, and see if we back up to
 * the beginning of the instruction.
 */
TEST_F(NCInstBytesTests, Reset) {
  /* First read some bytes into the bytes buffer. */
  NCInstBytesReadBytes(5, &_bytes);
  VerifyBytesMatchTestData();
  /* Now reset and see if we moved back. */
  NCInstBytesReset(&_bytes);
  EXPECT_EQ(0, _bytes.length) << "Reset didn't fix buffer length.";
  VerifyBytesMatchTestData();
  EXPECT_EQ(0, _memory.read_length) << "Reset didn't fix memory length.";
  EXPECT_EQ(0, _memory.overflow_count) << "Reset didn't fix memory overflow.";
  EXPECT_EQ((void*) _buffer, (void*) _memory.cur_pos)
      << "Reset did not reset memory.";
}

}  // anonymous namespace

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
