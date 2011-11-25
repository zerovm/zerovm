/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

// Unit tests for code in nc_inst_state.cc (and nc_inst_state_statics.c).

#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error("This file is not meant for use in the TCB")
#endif

// To turn on debugging of instruction decoding, change value of
// DEBUGGING to 1.
#define DEBUGGING 0

#include "gtest/gtest.h"
#include "include/nacl_macros.h"
#include "src/validator/x86/decoder/nc_inst_state.h"
#include "src/validator_x86/ncdis_decode_tables.h"
/* d'b: included to replace kNaClDecoderTables with kNaClValDecoderTables
   which will avoid of using some extra sources needless for sandbox
   also the compilation script for this test was changed (3 libs were
   removed from the linker options) */
#include "src/validator/x86/ncval_reg_sfi/ncval_decode_tables.h"
/* d'b end */

// Include static functions, so that we can test.
extern "C" {
#include "src/validator/x86/decoder/nc_inst_state_statics.c"
}

namespace {

// Size of buffer to use to contain bytes of an instruction.
static const size_t kBufferSize = 24;

// Test harness for routines in nc_inst_state.c and nc_inst_state_statics.c.
class NcInstStateTests : public ::testing::Test {
 protected:
  NcInstStateTests();
  void SetUp();
  void TearDown();

  // Plant the given byte as the next input byte in the input buffer.
  // Uses plant_index to determine the current end of the input buffer.
  void Plant(uint8_t byte);

  // Reset test state to a cleared input buffer, and (re)initialize
  // the instruction state.
  void Reset();

  // Routine to add dummy calls so that compilation errors are not defined
  // for static routines we have not tested.
  void dummy();

  // Reinitializes instruction state.
  void ResetState();

  // Resets the instruction pattern, and its flags to a default initial
  // state.
  void ResetInstPattern();

  // Fills the input buffer with unlikely bytes, and initializes
  // the reader to the beginning of the input buffer.
  void ResetInput();

  // Fills the input buffer with unlikely bytes, and set the plant
  // index to the beginning of the input buffer.
  void ResetInputBuffer();

  // Verify that we have consumed the given number of prefix bytes, with
  // the given number of rex prefixes, and that the prefix mask is set
  // to the given mask.
  //
  // Parameters are:
  //   num_bytes - Number of prefix bytes read.
  //   num_rex   - Number of prefix bytes that were rex prefixes.
  //   mask      - prefix mask that should have been generated.
  void VerifyConsumedPrefixBytes(uint8_t num_bytes, uint8_t num_rex,
                                 uint32_t mask);

  // Run tests that verify that the call to NaClConsume0F38XXNaClInstBytes
  // behaved as expected. Assumes the call was made through a call
  // to NaClConsumeInstBytes.
  void VerifyConsume0F38XXInstructions();

  // Run tests that verify that the call to NaClConsume0F3AXXNaClInstBytes
  // behaved as expected. Assumes the call was made through a call
  // to NaClConsumeInstBytes.
  void VerifyConsume0F3AXXInstructions();

  // Run tests that verify that the call to NaClConsume0FXXNaClInstBytes
  // behaved as expected.  Assumes the call was made through a call
  // to NaClConsumeInstBytes.
  void VerifyConsume0FXXInstructions();

  // Run tests that verify that the call to NaClConsumeX87NaClInstBytes
  // behaved as expected.  Assumes the call was made through a call
  // to NaClConsumeInstBytes.
  void VerifyConsumeX87Instructions();

  // Run tests that verify that the call to NaClConsumeInstBytes consumed
  // a single byte.
  void VerifyConsumeOneByteInstructions();

  // The instruction state to test.
  struct NaClInstState* _state;
  // The instruction iterator to use.
  struct NaClInstIter* _iter;
  // The memory segment to test.
  struct NaClSegment _segment;
  // The memory buffer in the memory segment.
  uint8_t _buffer[kBufferSize];
  // The instruction pattern to match against.
  struct NaClInst _inst_pattern;
  // The index of where the next planted byte should
  // be added to the input buffer.
  size_t _plant_index;
};

// Helper function to convert Bool to bool. This function is defined
// to get around a visual studio warning for 64-bits, which causes
// our trybots to fail (in our build system, compiler warnings are converted
// to compiler errors).
static inline bool Bool2bool(Bool b) {
  return b ? true : false;
}

NcInstStateTests::NcInstStateTests() {
  ResetInputBuffer();
  NaClSegmentInitialize(_buffer, 0, kBufferSize, &_segment);
}

void NcInstStateTests::SetUp() {
  /* d'b: kNaClDecoderTables replaced by kNaClValDecoderTables */
  _iter = NaClInstIterCreate(kNaClValDecoderTables, &_segment);
  _state = NaClInstIterGetUndecodedState(_iter);
  ResetInput();
  ResetState();
}

void NcInstStateTests::TearDown() {
  NaClInstIterDestroy(_iter);
}

void NcInstStateTests::Reset() {
  ResetInput();
  ResetState();
}

void NcInstStateTests::ResetState() {
  NaClInstStateInit(_iter, _state);
  ResetInstPattern();
}

void NcInstStateTests::ResetInstPattern() {
  _inst_pattern.flags = NACL_EMPTY_IFLAGS;
  _state->inst = &_inst_pattern;
}

void NcInstStateTests::ResetInputBuffer() {
  // Fill input buffer with unlikely byte codes.
  for (size_t i = 0; i < kBufferSize; ++i) {
    _buffer[i] = 'X';
  }
  // Mark start point for planting data into
  // the input buffer.
  _plant_index = 0;
}

void NcInstStateTests::ResetInput() {
  ResetInputBuffer();
  NCInstBytesReset(&_state->bytes);
}

void NcInstStateTests::Plant(uint8_t byte) {
  // TODO(Karl): Why do we get a compile time error if we use ASSERT.
  ASSERT_LT(_plant_index, kBufferSize) <<
      "Planted too many bytes, buffer overflow!";
  _buffer[_plant_index++] = byte;
  // Need to reset memory so that peek byte is set.
  NCRemainingMemoryReset(_state->bytes.memory);
}

void NcInstStateTests::VerifyConsumedPrefixBytes(
    uint8_t num_bytes, uint8_t num_rex, uint32_t mask) {
  EXPECT_EQ(num_bytes, _state->bytes.length);
  EXPECT_EQ(num_bytes, _state->num_prefix_bytes);
  EXPECT_EQ(mask, _state->prefix_mask);
  EXPECT_EQ(num_rex, _state->num_rex_prefixes);
}

void NcInstStateTests::VerifyConsume0F38XXInstructions() {
  NaClInstPrefixDescriptor desc;
  uint32_t prefix_mask = _state->prefix_mask;
  // Note: This code assumes that the prefix mask may have
  // other flags set before this routine is called. Hence,
  // we must be careful when updating and checking the
  // mask.

  // Test for all possible XX.
  for (int i = 0; i < NCDTABLESIZE; ++i) {
    // Test successfully matching 0f38XX
    _state->prefix_mask = prefix_mask;
    Plant(0x0f);
    Plant(0x38);
    Plant(i);
    NaClConsumeInstBytes(_state, &desc);
    if (NaClHasBit(_state->prefix_mask, kPrefixREP)) {
      EXPECT_EQ(NaClInstPrefixEnumSize, desc.matched_prefix);
    } else if (NaClHasBit(_state->prefix_mask, kPrefixREPNE)) {
      EXPECT_EQ(PrefixF20F38, desc.matched_prefix);
    } else if (NaClHasBit(_state->prefix_mask, kPrefixDATA16)) {
      EXPECT_EQ(Prefix660F38, desc.matched_prefix);
    } else {
      EXPECT_EQ(Prefix0F38, desc.matched_prefix);
    }
    EXPECT_EQ((uint8_t) i, desc.opcode_byte);
    EXPECT_EQ((uint8_t) 0, desc.next_length_adjustment);
    ResetInput();
    ResetState();
  }

  // Now verify if that there isn't an XX byte, things short curcuit correctly.
  _state->prefix_mask = prefix_mask;
  Plant(0x0f);
  Plant(0x38);
  _state->length_limit = 2;
  NaClConsumeInstBytes(_state, &desc);
  EXPECT_EQ(NaClInstPrefixEnumSize, desc.matched_prefix);
  EXPECT_EQ((uint8_t) 0, desc.next_length_adjustment);
  ResetInput();
  ResetState();
}

void NcInstStateTests::VerifyConsume0F3AXXInstructions() {
  NaClInstPrefixDescriptor desc;
  uint32_t prefix_mask = _state->prefix_mask;
  // Note: This code assumes that the prefix mask may have
  // other flags set before this routine is called. Hence,
  // we must be careful when updating and checking the
  // mask.

  // Test for all possible XX.
  for (int i = 0; i < NCDTABLESIZE; ++i) {
    // Test successfully matching 0F3AXX
    _state->prefix_mask = prefix_mask;
    Plant(0x0f);
    Plant(0x3a);
    Plant(i);
    NaClConsumeInstBytes(_state, &desc);
    if (NaClHasBit(_state->prefix_mask, kPrefixREP) ||
        NaClHasBit(_state->prefix_mask, kPrefixREPNE)) {
      EXPECT_EQ(NaClInstPrefixEnumSize, desc.matched_prefix);
    } else if (NaClHasBit(_state->prefix_mask, kPrefixDATA16)) {
      EXPECT_EQ(Prefix660F3A, desc.matched_prefix);
    } else {
      EXPECT_EQ(Prefix0F3A, desc.matched_prefix);
    }
    EXPECT_EQ((uint8_t) i, desc.opcode_byte);
    EXPECT_EQ((uint8_t) 0, desc.next_length_adjustment);
    ResetInput();
    ResetState();
  }

  // Now verify if that there isn't an XX byte, things short curcuit correctly.
  _state->prefix_mask = prefix_mask;
  Plant(0x0f);
  Plant(0x3a);
  _state->length_limit = 2;
  NaClConsumeInstBytes(_state, &desc);
  EXPECT_EQ(NaClInstPrefixEnumSize, desc.matched_prefix);
  EXPECT_EQ((uint8_t) 0, desc.next_length_adjustment);
  ResetInput();
  ResetState();
}

void NcInstStateTests::VerifyConsume0FXXInstructions() {
  NaClInstPrefixDescriptor desc;
  uint32_t prefix_mask = _state->prefix_mask;
  // Note: This code assumes that the prefix mask may have
  // other flags set before this routine is called. Hence,
  // we must be careful when updating and checking the
  // mask.

  // Test for all possible XX.
  for (int i = 0; i < NCDTABLESIZE; ++i) {
    if (i == 0x38 || i == 0x3a) continue;  // exclude special lookup cases.
    // Test successfully matching 0fXX
    _state->prefix_mask = prefix_mask;
    Plant(0x0f);
    Plant(i);
    NaClConsumeInstBytes(_state, &desc);
    if (NaClHasBit(_state->prefix_mask, kPrefixREP)) {
      if (NaClHasBit(_state->prefix_mask, kPrefixREPNE)) {
        EXPECT_EQ(NaClInstPrefixEnumSize, desc.matched_prefix);
      } else {
        EXPECT_EQ(PrefixF30F, desc.matched_prefix);
      }
    } else if (NaClHasBit(_state->prefix_mask, kPrefixREPNE)) {
      EXPECT_EQ(PrefixF20F, desc.matched_prefix);
    } else if (NaClHasBit(_state->prefix_mask, kPrefixDATA16)) {
      EXPECT_EQ(Prefix660F, desc.matched_prefix);
    } else {
      EXPECT_EQ(Prefix0F, desc.matched_prefix);
    }
    EXPECT_EQ((uint8_t) i, desc.opcode_byte);
    EXPECT_EQ((uint8_t) 0, desc.next_length_adjustment);
    ResetInput();
    ResetState();
  }

  // Now verify if that there isn't an XX byte, things short curcuit correctly.
  _state->prefix_mask = prefix_mask;
  Plant(0x0f);
  _state->length_limit = 1;
  NaClConsumeInstBytes(_state, &desc);
  EXPECT_EQ(NaClInstPrefixEnumSize, desc.matched_prefix);
  EXPECT_EQ((uint8_t) 0, desc.next_length_adjustment);
  ResetInput();
  ResetState();
}

void NcInstStateTests::VerifyConsumeX87Instructions() {
  NaClInstPrefixDescriptor desc;
  uint32_t prefix_mask = _state->prefix_mask;
  // Note: This code assumes that the prefix mask may have
  // other flags set before this routine is called. Hence,
  // we must be careful when updating and checking the
  // mask.

  // Try for all possible x87 initial bytes.
  for (uint8_t byte1 = 0xD8; byte1 <= 0xDF; ++byte1) {
    // Test for all possible XX.
    for (int i = 0; i < NCDTABLESIZE; ++i) {
      // Test successfully matching byte1 XX
      _state->prefix_mask = prefix_mask;
      Plant(byte1);
      Plant(i);
      NaClConsumeInstBytes(_state, &desc);
      NaClInstPrefix prefix = (NaClInstPrefix) (PrefixD8 + (byte1 - 0xD8));
      EXPECT_EQ(prefix, desc.matched_prefix);
      EXPECT_EQ((uint8_t) i, desc.opcode_byte);
      EXPECT_EQ((uint8_t) 0, desc.next_length_adjustment);
      ResetInput();
      ResetState();
    }

    // Now verify if that there isn't an XX byte, things short curcuit
    // correctly. For this context, it should return matching a single
    // byte instruction with no prefix.
    _state->prefix_mask = prefix_mask;
    Plant(byte1);
    _state->length_limit = 1;
    NaClConsumeInstBytes(_state, &desc);
    EXPECT_EQ(NoPrefix, desc.matched_prefix);
    EXPECT_EQ((uint8_t) 0, desc.next_length_adjustment);
    ResetInput();
    ResetState();
  }
}

void NcInstStateTests::VerifyConsumeOneByteInstructions() {
  NaClInstPrefixDescriptor desc;
  uint32_t prefix_mask = _state->prefix_mask;
  // Note: This code assumes that the prefix mask may have
  // other flags set before this routine is called. Hence,
  // we must be careful when updating and checking the
  // mask.

  // Test for all possible XX.
  for (int i = 0; i < NCDTABLESIZE; ++i) {
    // exclude special lookup cases.
    if (i == 0x0f || (i >= 0xD8 && i <= 0xDF)) continue;
    // Test successfully XX
    _state->prefix_mask = prefix_mask;
    Plant(i);
    NaClConsumeInstBytes(_state, &desc);
    EXPECT_EQ(NoPrefix, desc.matched_prefix);
    EXPECT_EQ((uint8_t) i, desc.opcode_byte);
    EXPECT_EQ((uint8_t) 0, desc.next_length_adjustment);
    ResetInput();
    ResetState();
  }

  // Now verify if that there isn't an XX byte, things short curcuit correctly.
  _state->prefix_mask = prefix_mask;
  _state->length_limit = 0;
  NaClConsumeInstBytes(_state, &desc);
  EXPECT_EQ(NaClInstPrefixEnumSize, desc.matched_prefix);
  EXPECT_EQ((uint8_t) 0, desc.next_length_adjustment);
  ResetInput();
  ResetState();
}

void NcInstStateTests::dummy() {
  NaClInstPrefixDescriptor prefix_desc;
  prefix_desc.opcode_byte = 0x0;
  prefix_desc.next_length_adjustment = 0;
  prefix_desc.matched_prefix = NoPrefix;
  NaClConsumeAndCheckOperandSize(_state);
  NaClConsumeAndCheckAddressSize(_state);
  NaClConsumeModRm(_state);
  NaClConsumeSib(_state);
  NaClConsumeDispBytes(_state);
  NaClConsumeImmediateBytes(_state);
  NaClValidatePrefixFlags(_state);
  NaClClearInstState(_state, 0);
  NaClGetNextInstCandidates(_state, &prefix_desc, NULL);
  NaClConsumeHardCodedNop(_state);
}

// Test function NaClExtactOpSize, which returns the expected
// number of bytes to represent operands.
TEST_F(NcInstStateTests, TestExtractOpSize) {
  // Test 32 amd 64 bit assumptions.

  // Test explicit size restrictors. Note: Only b should make a difference
  // in matching the pattern, since v, w, and o are used as excluders rather
  // than for matching (i.e. don't match unless operand size should be
  // 1).
  _inst_pattern.flags = NACL_IFLAG(OperandSize_b);
  EXPECT_EQ(1, NaClExtractOpSize(_state)) << "bytes are of size 1\n";
  _inst_pattern.flags = NACL_IFLAG(OperandSize_w);
  EXPECT_EQ(4, NaClExtractOpSize(_state));
  _inst_pattern.flags = NACL_IFLAG(OperandSize_v);
  EXPECT_EQ(4, NaClExtractOpSize(_state));
  _inst_pattern.flags = NACL_IFLAG(OperandSize_o);
  EXPECT_EQ(4, NaClExtractOpSize(_state));
  ResetState();

  // See if we interpret the Data16 prefix correctly.
  _state->prefix_mask = kPrefixDATA16;
  EXPECT_EQ(2, NaClExtractOpSize(_state));
  _inst_pattern.flags = NACL_IFLAG(SizeIgnoresData16);
  EXPECT_EQ(4, NaClExtractOpSize(_state));
  ResetState();

  // Test strictly 64-bit assumptions.
  if (NACL_TARGET_SUBARCH == 64) {
    // Check that we return a size 64 if the REX.W bit is set.
    for (uint8_t rex = NaClRexMin; rex <= NaClRexMax; ++rex) {
      _state->rexprefix = rex;
      if (NaClRexW(rex)) {
        EXPECT_EQ(8, NaClExtractOpSize(_state));
      } else {
        EXPECT_EQ(4, NaClExtractOpSize(_state));
      }
    }
    ResetState();

    // If we force the size to 64, it returns size 64.
    _inst_pattern.flags = NACL_IFLAG(OperandSizeForce64);
    EXPECT_EQ(8, NaClExtractOpSize(_state));
    ResetState();

    // Now repeat the tests, but with the default size set to 64 bits,
    // which replaces the default size of 4 with 8.

    // Test explicit size restrictors. Note: Only b should make a difference
    // in matching the pattern, since v, w, and o are used as excluders rather
    // than for matching (i.e. don't match unless operand size matches).
    _inst_pattern.flags =
        NACL_IFLAG(OperandSize_b) | NACL_IFLAG(OperandSizeDefaultIs64);
    EXPECT_EQ(1, NaClExtractOpSize(_state)) << "bytes are of size 1\n";
    _inst_pattern.flags =
        NACL_IFLAG(OperandSize_w) | NACL_IFLAG(OperandSizeDefaultIs64);
    EXPECT_EQ(8, NaClExtractOpSize(_state));
    _inst_pattern.flags =
        NACL_IFLAG(OperandSize_v) | NACL_IFLAG(OperandSizeDefaultIs64);
    EXPECT_EQ(8, NaClExtractOpSize(_state));
    _inst_pattern.flags =
        NACL_IFLAG(OperandSize_o) | NACL_IFLAG(OperandSizeDefaultIs64);
    EXPECT_EQ(8, NaClExtractOpSize(_state));
    ResetState();

    // See if we interpret the Data16 prefix correctly.
    _state->prefix_mask = kPrefixDATA16;
    _inst_pattern.flags = NACL_IFLAG(OperandSizeDefaultIs64);
    EXPECT_EQ(2, NaClExtractOpSize(_state));
    _inst_pattern.flags =
        NACL_IFLAG(SizeIgnoresData16) | NACL_IFLAG(OperandSizeDefaultIs64);
    EXPECT_EQ(8, NaClExtractOpSize(_state));
    ResetState();

    // Check that we return a size 64 independent of the REX.W bit.
    _inst_pattern.flags = NACL_IFLAG(OperandSizeDefaultIs64);
    for (uint8_t rex = NaClRexMin; rex <= NaClRexMax; ++rex) {
      _state->rexprefix = rex;
      EXPECT_EQ(8, NaClExtractOpSize(_state));
    }
  }
}

// Test function NaClExtractAddressSize, which returns the expected
// number of bits in operands corresponding to addresses.
TEST_F(NcInstStateTests, TestExtractAddressSize) {
  // Depending on whether we are in 32/64 bit mode, there are two
  // different address sizes.
  int small_address;
  int large_address;
  if (NACL_TARGET_SUBARCH == 64) {
    small_address = 32;
    large_address = 64;
  } else {
    small_address = 16;
    large_address = 32;
  }
  EXPECT_EQ(large_address, NaClExtractAddressSize(_state));
  _state->prefix_mask = kPrefixADDR16;
  EXPECT_EQ(small_address, NaClExtractAddressSize(_state));
}

extern "C" {
  // Define acceptable prefixes, and the corresponding flag that
  // should be set (except for rex prefixes).
  static const struct  prefix_pairs {
    uint8_t byte;
    uint32_t mask;
  } prefix_values[] = {
    {kValueSEGCS, kPrefixSEGCS},
    {kValueSEGSS, kPrefixSEGSS},
    {kValueSEGFS, kPrefixSEGFS},
    {kValueSEGGS, kPrefixSEGGS},
    {kValueDATA16, kPrefixDATA16},
    {kValueADDR16, kPrefixADDR16},
    {kValueREPNE, kPrefixREPNE},
    {kValueREP, kPrefixREP},
    {kValueLOCK, kPrefixLOCK},
    {kValueSEGES, kPrefixSEGES},
    {kValueSEGDS, kPrefixSEGDS}
  };
}

// Test function NaClConsumePrefixBytes to verify it only recognizes
// valid prefix values.
TEST_F(NcInstStateTests, ConsumesKnownPrefixBytes) {
  for (int byte = 0; byte < NCDTABLESIZE; ++byte) {
    bool byte_categorized = false;
    Plant(byte);
    EXPECT_TRUE(Bool2bool(NaClConsumePrefixBytes(_state)));
    if (NACL_TARGET_SUBARCH == 64 &&
        byte >= NaClRexMin && byte <= NaClRexMax) {
      VerifyConsumedPrefixBytes(1, 1, kPrefixREX);
      byte_categorized = true;
    } else {
      for (size_t j = 0; j < NACL_ARRAY_SIZE(prefix_values); ++j) {
        if (byte == prefix_values[j].byte) {
          VerifyConsumedPrefixBytes(1, 0, prefix_values[j].mask);
          byte_categorized = true;
        }
      }
    }
    if (!byte_categorized) {
      VerifyConsumedPrefixBytes(0, 0, 0);
    }
    ResetInput();
    ResetState();
  }
}

// Test function NaClConsumePrefixBytes to verify it can recognize
// pairs of non-rex prefix bytes.
TEST_F(NcInstStateTests, ConsumeNonRexPrefixBytePairs) {
  // First try some pairs within non-rex prefix bytes.
  for (size_t i = 0; i < NACL_ARRAY_SIZE(prefix_values) - 1; ++i) {
    Plant(prefix_values[i].byte);
    Plant(prefix_values[i+1].byte);
    EXPECT_TRUE(Bool2bool(NaClConsumePrefixBytes(_state)));
    VerifyConsumedPrefixBytes(2, 0,
                              prefix_values[i].mask | prefix_values[i+1].mask);
    ResetInput();
    ResetState();
  }
}

// Test Function NaClConsumePrefixBytes to verify it can recognize
// a Rex prefix followed by a non-rex prefix.
TEST_F(NcInstStateTests, ConsumeRexThenNonRexPrefixPairs) {
  if (NACL_TARGET_SUBARCH == 64) {
    // Try some pairs where one is rex.
    for (size_t i = 0; i < NACL_ARRAY_SIZE(prefix_values); ++i) {
      for (uint8_t rex = NaClRexMin; rex <= NaClRexMax; ++rex) {
        Plant(rex);
        Plant(prefix_values[i].byte);
        EXPECT_TRUE(Bool2bool(NaClConsumePrefixBytes(_state)));
        VerifyConsumedPrefixBytes(2, 1, prefix_values[i].mask | kPrefixREX);
        ResetInput();
        ResetState();
      }
    }
  }
}

// Test Function NaClConsumePrefixBytes to verify it can recognize
// a non-rex prefix, followed by a rex prefix.
TEST_F(NcInstStateTests, ConsumeNonRexThenRexPrefixPairs) {
  if (NACL_TARGET_SUBARCH == 64) {
    // Try some pairs where one is rex.
    for (size_t i = 0; i < NACL_ARRAY_SIZE(prefix_values); ++i) {
      for (uint8_t rex = NaClRexMin; rex <= NaClRexMax; ++rex) {
        Plant(prefix_values[i].byte);
        Plant(rex);
        EXPECT_TRUE(Bool2bool(NaClConsumePrefixBytes(_state)));
        VerifyConsumedPrefixBytes(2, 1, prefix_values[i].mask | kPrefixREX);
        ResetInput();
        ResetState();
      }
    }
  }
}

// Test function NaClConsumePrefixBytes on multiple rex prefixes.
TEST_F(NcInstStateTests, ConsumeMultipleRexPrefixes) {
  if (NACL_TARGET_SUBARCH == 64) {
    for (uint8_t rex1 = NaClRexMin; rex1 <= NaClRexMax; ++rex1) {
      for (uint8_t rex2 = NaClRexMin; rex2 <= NaClRexMax; ++rex2) {
        Plant(rex1);
        Plant(rex2);
        EXPECT_TRUE(Bool2bool(NaClConsumePrefixBytes(_state)));
        VerifyConsumedPrefixBytes(2, 2, kPrefixREX);
        ResetInput();
        ResetState();
      }
    }
  }
}

// Test function NaClConsumePrefixBytes to see if we allow multiple
// copies of the same (non-rex) prefix.
TEST_F(NcInstStateTests, ConsumeDuplicatePrefixes) {
  // Try with non rex prefixes.
  for (size_t i = 0; i < NACL_ARRAY_SIZE(prefix_values); ++i) {
    Plant(prefix_values[i].byte);
    Plant(prefix_values[i].byte);
    EXPECT_TRUE(Bool2bool(NaClConsumePrefixBytes(_state)));
    VerifyConsumedPrefixBytes(2, 0, prefix_values[i].mask);
    ResetInput();
    ResetState();
  }
}

// Test if we can recognize 14 prefix bytes.
TEST_F(NcInstStateTests, Consume14PrefixBytes) {
  for (int i = 0; i < 14; ++i) {
    Plant(kValueDATA16);
  }
  EXPECT_TRUE(Bool2bool(NaClConsumePrefixBytes(_state)));
  VerifyConsumedPrefixBytes(14, 0, kPrefixDATA16);
}

// Test that we can't accept 15 prefix bytes.
TEST_F(NcInstStateTests, Consume15PrefixBytes) {
  for (int i = 0; i < 15; ++i) {
    Plant(kValueDATA16);
  }
  EXPECT_TRUE(Bool2bool(NaClConsumePrefixBytes(_state)));
  EXPECT_EQ((uint8_t) 14, _state->bytes.length);
}

// Defines the set of prefix bytes that effect multibyte instructions
// (i.e. REP, REPNE,  and DATA16), and all possible combinations of
// these prefixes.
static const uint32_t kMultibytePrefixes[] = {
  0,
  kPrefixREP,
  kPrefixREP | kPrefixREPNE,
  kPrefixREP | kPrefixREPNE | kPrefixDATA16,
  kPrefixREPNE,
  kPrefixREPNE | kPrefixDATA16,
  kPrefixDATA16
};

// Test function NaClConsume0F38XXNaClInstBytes, as called through
// function NaClConsumeInstBytes.
TEST_F(NcInstStateTests, ConsumeOF38XXInstructions) {
  // First try effects of just multibyte prefixes.
  for (size_t i = 0; i < NACL_ARRAY_SIZE(kMultibytePrefixes); ++i) {
    _state->prefix_mask = kMultibytePrefixes[i];
    VerifyConsume0F38XXInstructions();

    // Verify that adding a rex prefix don't effect anything.
    _state->prefix_mask = kMultibytePrefixes[i] | kPrefixREX;
    VerifyConsume0F38XXInstructions();

    // Now try adding other possible prefixes to see if they break anything.
    for (size_t j = 0; j < NACL_ARRAY_SIZE(prefix_values); ++j) {
      _state->prefix_mask = kMultibytePrefixes[i] | prefix_values[i].mask;
      VerifyConsume0F38XXInstructions();

      // Verify that adding a rex prefix don't effect anything.
      _state->prefix_mask = kMultibytePrefixes[i] | prefix_values[i].mask
          | kPrefixREX;
      VerifyConsume0F38XXInstructions();
    }
  }
}

// Test function NaClConsume0F3AXXNaClInstBytes, as called through
// function NaClConsumeInstBytes.
TEST_F(NcInstStateTests, ConsumeOF3AXXInstructions) {
  // First try effects of just multibyte prefixes.
  for (size_t i = 0; i < NACL_ARRAY_SIZE(kMultibytePrefixes); ++i) {
    _state->prefix_mask = kMultibytePrefixes[i];
    VerifyConsume0F3AXXInstructions();

    // Verify that adding a rex prefix don't effect anything.
    _state->prefix_mask = kMultibytePrefixes[i] | kPrefixREX;
    VerifyConsume0F3AXXInstructions();

    // Now try adding other possible prefixes to see if they break anything.
    for (size_t j = 0; j < NACL_ARRAY_SIZE(prefix_values); ++j) {
      _state->prefix_mask = kMultibytePrefixes[i] | prefix_values[i].mask;
      VerifyConsume0F3AXXInstructions();

      // Verify that adding a rex prefix don't effect anything.
      _state->prefix_mask = kMultibytePrefixes[i] | prefix_values[i].mask
          | kPrefixREX;
      VerifyConsume0F3AXXInstructions();
    }
  }
}

// Test function NaClConsume0FXXNaClInstBytes, as called through
// function NaClConsumeInstBytes.
TEST_F(NcInstStateTests, ConsumeOFXXInstructions) {
  // First try effects of just multibyte prefixes.
  for (size_t i = 0; i < NACL_ARRAY_SIZE(kMultibytePrefixes); ++i) {
    _state->prefix_mask = kMultibytePrefixes[i];
    VerifyConsume0FXXInstructions();

    // Verify that adding a rex prefix don't effect anything.
    _state->prefix_mask = kMultibytePrefixes[i] | kPrefixREX;
    VerifyConsume0FXXInstructions();

    // Now try adding other possible prefixes to see if they break anything.
    for (size_t j = 0; j < NACL_ARRAY_SIZE(prefix_values); ++j) {
      _state->prefix_mask = kMultibytePrefixes[i] | prefix_values[i].mask;
      VerifyConsume0FXXInstructions();

      // Verify that adding a rex prefix don't effect anything.
      _state->prefix_mask = kMultibytePrefixes[i] | prefix_values[i].mask
          | kPrefixREX;
      VerifyConsume0FXXInstructions();
    }
  }
}

// Test function NaClConsumeX87NaClInstBytes, as called through
// function NaClConsumeInstBytes.
TEST_F(NcInstStateTests, ConsumeX87Instructions) {
  // First try effects of just multibyte prefixes.
  for (size_t i = 0; i < NACL_ARRAY_SIZE(kMultibytePrefixes); ++i) {
    _state->prefix_mask = kMultibytePrefixes[i];
    VerifyConsumeX87Instructions();

    // Verify that adding a rex prefix don't effect anything.
    _state->prefix_mask = kMultibytePrefixes[i] | kPrefixREX;
    VerifyConsumeX87Instructions();

    // Now try adding other possible prefixes to see if they break anything.
    for (size_t j = 0; j < NACL_ARRAY_SIZE(prefix_values); ++j) {
      _state->prefix_mask = kMultibytePrefixes[i] | prefix_values[i].mask;
      VerifyConsumeX87Instructions();

      // Verify that adding a rex prefix don't effect anything.
      _state->prefix_mask = kMultibytePrefixes[i] | prefix_values[i].mask
          | kPrefixREX;
      VerifyConsumeX87Instructions();
    }
  }
}

// Test function NaClConsumeInstBytes for one byte instruction values.
TEST_F(NcInstStateTests, ConsumeOneByteInstructions) {
  // First try effects of just multibyte prefixes.
  for (size_t i = 0; i < NACL_ARRAY_SIZE(kMultibytePrefixes); ++i) {
    _state->prefix_mask = kMultibytePrefixes[i];
    VerifyConsumeOneByteInstructions();

    // Verify that adding a rex prefix don't effect anything.
    _state->prefix_mask = kMultibytePrefixes[i] | kPrefixREX;
    VerifyConsumeOneByteInstructions();

    // Now try adding other possible prefixes to see if they break anything.
    for (size_t j = 0; j < NACL_ARRAY_SIZE(prefix_values); ++j) {
      _state->prefix_mask = kMultibytePrefixes[i] | prefix_values[i].mask;
      VerifyConsumeOneByteInstructions();

      // Verify that adding a rex prefix don't effect anything.
      _state->prefix_mask = kMultibytePrefixes[i] | prefix_values[i].mask
          | kPrefixREX;
      VerifyConsumeOneByteInstructions();
    }
  }
}

}  // anonymous namespace

int main(int argc, char *argv[]) {
  NaClLogModuleInit();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
