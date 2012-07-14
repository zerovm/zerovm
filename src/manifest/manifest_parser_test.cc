/*
 * manifest_parser_test.c
 * functions to test: ManifestCtor(), ManifestDtor(), GetValuByKey(),
 * the test create temporary file: "manifest_1.txt"
 *
 *  Created on: Nov 12, 2011
 *      Author: d'b
 */
#include <stdio.h>
#include <stdlib.h>
#include "gtest/gtest.h"
#include "src/manifest/manifest_parser.h"

#define MANIFEST_FILE "killme.manifest.txt"
#define MANIFEST_DATA \
      "-=-=-=-=-=-=-=-=-=-=-=- just a test -=-=-=-=-=-=-=-=-=-=-=-\n"\
      "== this is manifest example\n\n"\
      "key01 = value01\n"\
      "  key02  =  value02\n"\
      "key03=value03 \n"\
      "       key 014=value 014\n"\
      "key05 =value05\n"\
      "      key06= value06\n"\
      "key =\n"\
      "  = value_a\n"\
      "  =\r"\
      "key value_b\n"\
      "\t \t key07\t\t\t\t= \t\t\tvalue07   \n"

// test whole manifest processing and get value by key
TEST(ManifestTests, ManifestParserTest)
{
  FILE *f;

  // prepare mock
  if((f = fopen(MANIFEST_FILE, "w")) == NULL) return;
  fprintf(f, MANIFEST_DATA);
  fclose(f);
  EXPECT_EQ(0, ManifestCtor(MANIFEST_FILE));

  // go through manifest data
  EXPECT_STREQ("value 014", GetValueByKey((char*)"key 014"));
  EXPECT_STREQ("value01", GetValueByKey((char*)"key01"));
  EXPECT_STREQ("value07", GetValueByKey((char*)"key07"));
  EXPECT_STREQ("value05", GetValueByKey((char*)"key05"));
  EXPECT_STREQ("value02", GetValueByKey((char*)"key02"));
  EXPECT_STREQ("value03", GetValueByKey((char*)"key03"));
  EXPECT_STREQ("value06", GetValueByKey((char*)"key06"));

  // check invalid keys
  // ### put the death test here
//  EXPECT_STREQ(NULL, GetValueByKey(NULL));
//  EXPECT_STREQ(NULL, GetValueByKey((char*)""));
//  EXPECT_STREQ(NULL, GetValueByKey((char*)"key"));
//  EXPECT_STREQ(NULL, GetValueByKey((char*)" "));

  // free resources
  ManifestDtor();
  remove(MANIFEST_FILE);

  // check if manifest constructed no more
  // ### put the death test here
//  EXPECT_STREQ(NULL, GetValueByKey((char*)"key 014"));
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
