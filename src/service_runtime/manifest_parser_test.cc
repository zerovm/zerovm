/*
 * manifest_parser_test.c
 * unit test over google testing framework
 * the test create temporary file: "manifest_1.txt"
 *
 *  Created on: Nov 12, 2011
 *      Author: d'b
 */
#include <stdio.h>
#include <string.h>
#include "gtest/gtest.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/manifest_parser.h"

#define MANIFEST_FILE "manifest_1.txt"

/*
 * a - string to process, b - sample, func - procedure to invoke
 * b can be NULL, a cannot be NULL, but can be ""
 */
 /* get rid of warning about NULL usage */
#define CHECK(b, a, func) \
  do { \
    char *buf = (char*)malloc(1025); \
    strncpy(buf, a, 1024); \
    if(b == NULL) \
      EXPECT_EQ(NULL, func(buf)); \
    else \
      EXPECT_EQ(0, strncmp(b, func(buf), 1024)); \
    free(buf); \
  } while(0)

/* code doubling. i need extend CHECK */
#define CHECK2(b, a, func) \
  do { \
    char *buf = (char*)malloc(1025); \
    strncpy(buf, a, 1024); \
    if(b == NULL) \
      EXPECT_EQ(NULL, func(&stat, buf)); \
    else \
      EXPECT_EQ(0, strncmp(b, func(&stat, buf), 1024)); \
    free(buf); \
  } while(0)

/* just to fix warning: null argument where non-null required */
const char *NIL = (const char*)0;

// Test harness for routines in manifest_parse.c.
class ManifestTests : public ::testing::Test {
 protected:
  ManifestTests() {}
};

// test cutting spaces
TEST_F(ManifestTests, CutSpacesTest)
{
  EXPECT_EQ(NIL, CutSpaces(NULL));
  CHECK(NIL, "", CutSpaces);
  CHECK("1-way_test", "1-way_test   ", CutSpaces);
  CHECK("2-way test", "2-way test   ", CutSpaces);
  CHECK("3-way test", "\t 3-way test\t \t", CutSpaces);
  CHECK("2-way test", "\t 2-way test", CutSpaces);
  CHECK("1-way_test", "   1-way_test", CutSpaces);
  CHECK("1-way test", "1-way test", CutSpaces);
  CHECK("0-way_test", "0-way_test", CutSpaces);
}

// test key extraction
TEST_F(ManifestTests, GetKeyTest)
{
  EXPECT_EQ(NULL, GetKey(NULL));
  CHECK(NIL, "", GetKey);
  CHECK(NIL, "invalid key  value", GetKey);
  CHECK("key", "key=value", GetKey);
  CHECK("key", "\tkey = \tvalue", GetKey);
  CHECK("complex key", " \tcomplex key = \tvalue", GetKey);
  CHECK("very \tcomplex key", " \tvery \tcomplex key=value", GetKey);
  CHECK("complex key", "complex key=value", GetKey);
}

// test value extraction
TEST_F(ManifestTests, GetValueTest)
{
  EXPECT_EQ(NULL, GetValue(NULL));
  CHECK(NIL, "", GetValue);
  CHECK(NIL, "== invalid value", GetValue);
  CHECK("value", "key=value", GetValue);
  CHECK("value", "\tkey = \tvalue", GetValue);
  CHECK("complex value", " \tcomplex key = \tcomplex value ", GetValue);
  CHECK("very\tcomplex\tvalue", " \tvery \tcomplex key= very\tcomplex\tvalue\t", GetValue);
  CHECK("complex value", "complex key=complex value", GetValue);
}

// test whole manifest processing and get value by key
TEST_F(ManifestTests, ParseManifestTest)
{
  struct NaClApp stat;
  FILE *f;

  if((f = fopen(MANIFEST_FILE, "w")) == NULL)
    return; // i need to solve this. how to safe create/open file for test?

  // create file for test
  fprintf(f,
      "-=-=-=-=-=-=-=-=-=-=-=- just a test -=-=-=-=-=-=-=-=-=-=-=-\n"
      "== this is manifest example\n\n"
      "key01 = value01\n"
      "  key02  =  value02\n"
      "key03=value03 \n"
      "       key 014=value 014\n"
      "key05 =value05\n"
      "      key06= value06\n"
      "key =\n"
      "  = value_a\n"
      "  =\r"
      "key value_b\n"
      "\t \t key07\t\t\t\t= \t\t\tvalue07   \n");
  fclose(f);

  EXPECT_EQ(7, ParseManifest(MANIFEST_FILE, &stat)); // get rid of warning
  CHECK2("value 014", "key 014", GetValueByKey);
  CHECK2("value01", "key01", GetValueByKey);
  CHECK2("value07", "key07", GetValueByKey);
  CHECK2("value05", "key05", GetValueByKey);
  CHECK2("value02", "key02", GetValueByKey);
  CHECK2("value03", "key03", GetValueByKey);
  CHECK2("value06", "key06", GetValueByKey);
  CHECK2(NIL, "not existing key", GetValueByKey);
  remove(MANIFEST_FILE); /* remove test file */
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
