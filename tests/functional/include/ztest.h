/*
 * ztest.h
 *
 *  Created on: Feb 26, 2013
 *      Author: bortoq
 */

#ifndef ZTEST_H_
#define ZTEST_H_

static int errcount = 0;
#ifndef ERRCOUNT
#define ERRCOUNT errcount
#endif

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

#endif /* ZTEST_H_ */
