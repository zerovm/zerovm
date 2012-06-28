/*
 * zvm_manifest.c
 *
 *  Created on: Dec 2, 2011
 *      Author: d'b
 */

#include <string.h>
#include <stdio.h>

#define USER_SIDE
#include "zvm.h"

/* pointer to trap() through the trampoline */
int32_t (*_trap)(uint64_t *in) = (int32_t (*)(uint64_t*))
    0x10000 /* start of trampoline */ +
    0x20 /* size of trampoline record */ *
    0 /* onering syscall number */;

/* wrapper for zerovm "TrapUserSetup" */
int32_t zvm_setup(struct UserManifest *hint)
{
  uint64_t request[] = {TrapUserSetup, 0, (uint32_t)hint};
  return _trap(request);
}

/* wrapper for zerovm "TrapRead" */
int32_t zvm_pread(int desc, char *buffer, int32_t size, int64_t offset)
{
  uint64_t request[] = {TrapRead, 0, desc, (uint32_t)buffer, size, offset};
  return _trap(request);
}

/* wrapper for zerovm "TrapWrite" */
int32_t zvm_pwrite(int desc, char *buffer, int32_t size, int64_t offset)
{
  uint64_t request[] = {TrapWrite, 0, desc, (uint32_t)buffer, size, offset};
  return _trap(request);
}

/* wrapper for zerovm "TrapExit" */
int32_t zvm_exit(int32_t code)
{
  uint64_t request[] = {TrapExit, 0, code};
  return _trap(request);
}

#undef USER_SIDE
