/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * update (d'b): minor fixes due refactoring
 */

/*
 * Test for nacl logging.
 *  Author: YaroslavLitvinov
 *  Date: 7.03.2012
 */

#include <stdlib.h>
#include <limits.h>

#include "gtest/gtest.h"
#include "src/utils/tools.h"
#include "include/nacl_macros.h"
#include "include/nacl_platform.h"
#include "src/platform/nacl_log.h"
#include "src/gio/gio.h"

#define NACL_LOG_TEST_FILE "nacl_log_test.log"

// Test harness for routines in manifest_parse.c.
class LoggingTests: public ::testing::Test
{
public:
  void LogTestVerboseAboveOrEqualThanDetailsLevel();
  void LogTestVerboseLessThanDetailsLevel();
  void LogTestFatal();
  void LogTestFormatString();
protected:

  // Sets up the test fixture.
  void SetUp();

  // Tears down the test fixture.
  void TearDown();

  // return bytes count really written
  int TestWriteLog(int detail_level, int verbose, const char* logstr);
};

void LoggingTests::SetUp()
{
  const char* logfilename = NACL_LOG_TEST_FILE;

  // remove old log file
  remove(logfilename);

  // create log
  NaClLogSetFile(logfilename);
}

void LoggingTests::TearDown()
{
  const char* logfilename = NACL_LOG_TEST_FILE;

  // destroy log object
  NaClLogModuleFini();

  // remove log file
  remove(logfilename);
}

int LoggingTests::TestWriteLog(int detail_level, int verbose, const char* logstr)
{
  NaClLogSetVerbosity(verbose);
  uint64_t logfile_size_before = GetFileSize(NACL_LOG_TEST_FILE);

  // "" added to avoid warning -Wformat-security
  NaClLog(detail_level, logstr, "");
  uint64_t logfile_size_after = GetFileSize(NACL_LOG_TEST_FILE);
  return logfile_size_after - logfile_size_before;
}

TEST_F(LoggingTests, LogTestVerboseAboveOrEqualThanDetailsLevel)
{
  // Fail test if no data logged but should be, because  verbose >= detail level
  // logging was redirected to syslog. which discarded this tests
#if 0
  EXPECT_TRUE(TestWriteLog(0, 0, "detail_level-0 verbose-0"));
  EXPECT_TRUE(TestWriteLog(LOG_INFO, 0, "detail_level-LOG_INFO verbose-0"));
  EXPECT_TRUE(TestWriteLog(LOG_WARNING, 0, "detail_level-LOG_WARNING verbose-0"));
  EXPECT_TRUE(TestWriteLog(LOG_ERROR, 0, "detail_level-LOG_ERROR verbose-0"));
  EXPECT_TRUE(TestWriteLog(0, 1, "detail_level-0 verbose-1"));
  EXPECT_TRUE(TestWriteLog(0, INT_MAX-1, "detail-0 verbose-INT_MAX-1"));
  EXPECT_TRUE(TestWriteLog(INT_MAX-1, INT_MAX-1, "detail-INT_MAX-1 verbose-INT_MAX-1"));
#endif
}

TEST_F(LoggingTests, LogTestVerboseLessThanDetailsLevel)
{
  // Fail test if data logged but should not be, because detail level > verbose
  EXPECT_FALSE(TestWriteLog(1, 0, "detail_level-1 verbose-0"));
  EXPECT_FALSE(TestWriteLog(INT_MAX, 0, "detail-INT_MAX verbose-0"));
}

TEST_F(LoggingTests, LogTestFormatString)
{
  // init verbose
  NaClLogSetVerbosity(0);

  // test basic format string
  // Test passed if don't crashed
  NaClLog(0, "log test int %d", 1);
  NaClLog(0, "log test str %s", NACL_LOG_TEST_FILE);
}

// TEST_F(LoggingTests, LogTestFatal) {
//   // LOG_FATAL should log data and abort application running
//   TestWriteLog(LOG_FATAL, 0, "LOG_FATAL");
//   ASSERT_TRUE(0);
// }

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
