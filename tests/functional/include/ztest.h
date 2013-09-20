/*
 * Copyright (c) 2012, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ZTEST_H_
#define ZTEST_H_

static int errcount = 0;
#ifndef ERRCOUNT
#define ERRCOUNT errcount
#endif

#define PAGESIZE 0x10000
#define ROUNDDOWN_64K(a) ((a) & ~(PAGESIZE - 1LLU))
#define ROUNDUP_64K(a) ROUNDDOWN_64K((a) + PAGESIZE - 1LLU)

#define UNREFERENCED_VAR(a) do { (void)a; } while(0)
#define UNREFERENCED_FUNCTION(f) do {if(0) f();} while(0)

#define ZTEST(cond) \
  do {\
    if(cond)\
      FPRINTF(STDERR, "succeed on %3d: %s\n", __LINE__, #cond);\
    else\
    {\
      FPRINTF(STDERR, "failed on %4d: %s\n", __LINE__, #cond);\
      ++ERRCOUNT;\
    }\
  } while(0)

/* count errors and exit with it */
#define ZREPORT \
  do { \
    if(ERRCOUNT > 0) \
      FPRINTF(STDERR, "TEST FAILED with %d errors\n", ERRCOUNT); \
    else \
      FPRINTF(STDERR, "TEST SUCCEED\n\n"); \
    exit(ERRCOUNT); \
  } while(0)

/* works like ZTEST but ends the test execution after error */
#define ZFAIL(cond) \
  do { \
    ZTEST(cond); \
    if(!(cond)) ZREPORT; \
  } while(0)

#endif /* ZTEST_H_ */
