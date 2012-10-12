/*
 * manifest_parser_test.c
 * functions to test: ManifestCtor(), ManifestDtor(), GetValuByKey(),
 *
 *  Created on: Nov 12, 2011
 *      Author: d'b
 */
#include <stdio.h>
#include <stdlib.h>
#include "gtest/gtest.h"
#include "src/manifest/manifest_parser.h"
#include "src/service_runtime/tools.h"

#define BIG_ENOUGH 0x10000
#define MANIFEST_FILE "killme.manifest.txt"
#define MANIFEST_DATA \
      "-=-=-=-=-=-=-=-=-=-=-=- just a test -=-=-=-=-=-=-=-=-=-=-=-\n"\
      "== this is manifest example\n\n"\
      "key01 = value01\n"\
      "  key02  =  value02\n"\
      "key03=value03 \r"\
      "       key 014=value 014\n"\
      "key05 =value05\n"\
      "      key06= value06\n"\
      "key =\n"\
      "  = value_a\n"\
      "  =\r"\
      "key value_b\n"\
      "\t \t key07\t\t\t\t= \t\t\tvalue07   \n"\
      "key_with_multivalues1 = value1, value2, value3,value4\n"\
      "key_with_multivalues2 = value1 value2   value3\tvalue4\n"\
      "multikey = value1, value2\n"\
      "multikey = value3,  value4 \n"

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

  // check the multikey case
  char buf[BIG_ENOUGH], **tokens = (char**)buf;
  int code = 0;
  code = GetValuesByKey("multikey", tokens, BIG_ENOUGH);
  EXPECT_EQ(2, code);
  EXPECT_STREQ("value1, value2", tokens[0]);
  EXPECT_STREQ("value3,  value4", tokens[1]);

  // check the multivalues case
  char str1[] = "value1, value2, value3,value4\n";
  code = ParseValue(str1, " ,\r\n", tokens, 4);
  EXPECT_STREQ(tokens[0], "value1");
  EXPECT_STREQ(tokens[1], "value2");
  EXPECT_STREQ(tokens[2], "value3");
  EXPECT_STREQ(tokens[3], "value4");

  char str2[] = "value1 value2   value3\tvalue4";
  code = ParseValue(str2, " \t", tokens, 4);
  EXPECT_STREQ(tokens[0], "value1");
  EXPECT_STREQ(tokens[1], "value2");
  EXPECT_STREQ(tokens[2], "value3");
  EXPECT_STREQ(tokens[3], "value4");

  // test manifest destructor
  // todo(d'b): put the death test here
  ManifestDtor();
  remove(MANIFEST_FILE);
//  EXPECT_STREQ(NULL, GetValueByKey((char*)"key 014"));
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
