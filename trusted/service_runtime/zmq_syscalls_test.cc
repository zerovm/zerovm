/*
 * zmq_syscalls_test.cc
 * unit test over google testing framework
 * the test create temporary file: "killme"
 *
 *  Created on: Nov 14, 2011
 *      Author: d'b
 */

// test disabled in makefile until completion. i need properly initialize naclapp struct

// in order to complete writing this code i need
// nameservice to detect nacl file descriptor by name

#include <stdio.h>
#include <string.h>
#include "gtest/gtest.h"
#include "trusted/service_runtime/include/sys/fcntl.h"
#include "trusted/service_runtime/sel_ldr.h"
#include "trusted/service_runtime/zmq_syscalls.h"
#include <assert.h>

#define TEST_FILE "http://killme"

// Test harness for routines in manifest_parse.c
class ZMQTests : public ::testing::Test {
 protected:
  ZMQTests()
  {
    int code = NaClAppCtor(&app);
    assert(1 == code);
  }

  // we need to test 4 functions with the same file and NaClApp
  struct NaClApp app;
};

// test swift url recognition
TEST_F(ZMQTests, IsSwiftURLTest)
{
  EXPECT_EQ(1, IsSwiftURL("http://name"));
  EXPECT_EQ(1, IsSwiftURL("hTtP://name"));
  EXPECT_EQ(0, IsSwiftURL("somename"));
}

// test zmq open
TEST_F(ZMQTests, ZMQSysOpenTest)
{
  struct NaClAppThread *natp; // i need construct proper nap and put it to natp
  int code;

  natp->nap = &app;

  // natp will be updated with a new opened file
  code = ZMQSysOpen(natp, (char*)TEST_FILE, NACL_ABI_O_RDONLY, 0);
  EXPECT_EQ(0, code);
}

// test zmq read
TEST_F(ZMQTests, ZMQSysReadTest)
{
  struct NaClAppThread *natp; // i need construct proper nap and put it to natp
  int code;

  natp->nap = &app;

  // natp must already contain opened file
  //code = ZMQSysRead(natp, TEST_FILE, NACL_ABI_O_RDONLY, 0);
  EXPECT_EQ(0, code);
}
// test zmq write
// test zmq close

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
