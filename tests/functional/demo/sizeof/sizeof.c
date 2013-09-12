/*
 * sizeof of main types
 * differences between host (x86-64) and guest (nacl_x86-64)
 * type           host guest
 * long           8    4
 * long double    16   8
 * void*          8    4 (any pointer)
 */
#ifdef __ZEROVM__
#include "include/zvmlib.h"
#else
#include <stdio.h>
#endif

#define DUMP(s) printf("%24s = %lu\n", #s, s)
#define REM(s) printf("\n/* %22s */\n", #s)

typedef enum {_} ENUM;
typedef struct {int _;} STRUCT;
typedef union {int _;} UNION;

int main()
{
  REM(pointers);
  DUMP(sizeof(char*));
  DUMP(sizeof(unsigned char*));
  DUMP(sizeof(int*));
  DUMP(sizeof(unsigned int*));
  DUMP(sizeof(long*));
  DUMP(sizeof(unsigned long*));
  DUMP(sizeof(void*));
  DUMP(sizeof(float*));
  DUMP(sizeof(double*));
  DUMP(sizeof(long double*));
  DUMP(sizeof(long long*));

  REM(main types);
  DUMP(sizeof(char));
  DUMP(sizeof(unsigned char));
  DUMP(sizeof(int));
  DUMP(sizeof(unsigned int));
  DUMP(sizeof(long));
  DUMP(sizeof(unsigned long));
  DUMP(sizeof(void));
  DUMP(sizeof(float));
  DUMP(sizeof(double));
  DUMP(sizeof(long double));
  DUMP(sizeof(long long));

  REM(custom examples);
  DUMP(sizeof(ENUM));
  DUMP(sizeof(STRUCT));
  DUMP(sizeof(UNION));
  DUMP(sizeof(main));
  DUMP(sizeof(&main));
  DUMP(sizeof(&&label));
  DUMP(sizeof("1234567"));
  DUMP(sizeof('1'));
  DUMP(sizeof(1));
  DUMP(sizeof(1LL));
  DUMP(sizeof(char[0]));
label:
  return 0;
}
