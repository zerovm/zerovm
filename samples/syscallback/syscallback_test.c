/*
 * syscallback test
 *
 *  Created on: Dec 11, 2011
 *      Author: d'b
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "api/zvm.h"

#define LOGFIX /* temporary fix until zrt library will be finished */

/*
 * this function uninstall itself and show "bye" message
 * note: it takes argument from the main() via zvm interceptor
 */
int Callback(int32_t arg)
{
  struct SetupList setup;

  /* uninstall callback function */
  setup.syscallback = 0;
  zvm_setup(&setup);

  /* say good bye */
  log_msg("good bye cruel world..\n");
  exit(0);

  /* not reachable */
  return arg;
}

int main()
{
  struct SetupList setup;
  int retcode = ERR_CODE;

  /* install callback function */
  setup.syscallback = (int32_t)Callback;
  zvm_setup(&setup);

#ifdef LOGFIX
  /* set up the log */
  retcode = log_set(&setup);
  if(retcode) return retcode;
#endif

  /* say hello */
  log_msg("hello! syscallback program started\n");

  /* invoke puts with pointer to setup instead of string */
  puts((char*)&setup);

  return retcode;
}
