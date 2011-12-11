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

#include "/home/dazo/git/zerovm/api/zvm_manifest.h"

/*
 * this function uninstall itself and show "hi" message
 * note: it takes argument from the main() via zvm interceptor
 */
int Callback(int32_t arg)
{
  struct SetupList setup;

  /* uninstall callback function */
  setup.syscallback = 0;
  zvm_setup(&setup);

  fprintf(stderr, "goodbye cruel world..\n");
  exit(1);

  return arg;
}

int main()
{
  struct SetupList setup;

  /* install callback function */
  setup.syscallback = (int32_t)Callback;
  zvm_setup(&setup);

  /* invoke puts with pointer to setup instead of string */
  puts((char*)&setup);

  return 0;
}
