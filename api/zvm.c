/*
 * ZeroVM API. contain zerovm api functions
 * notice: this is an untrusted code
 *
 *  Created on: Dec 2, 2011
 *      Author: d'b
 */

#include <string.h>
#include <stdio.h>

#ifndef USER_SIDE
#define USER_SIDE
#endif
#include "zvm.h"

/* pointer to trap() through the trampoline */
static int32_t (*_trap)(uint64_t *in) = (int32_t (*)(uint64_t*))
    0x10000 /* start of trampoline */ +
    0x20 /* size of trampoline record */ *
    0 /* onering syscall number */;

/* zerovm error number */
static int32_t zvm_errno_num = 0;

/* zerovm error code accessor */
int32_t zvm_errno()
{
  return zvm_errno_num;
}

/* initialization of zerovm api */
struct UserManifest *zvm_init()
{
  return (void*) *((uintptr_t*) 0xFEFFFFFC);
}

/*
 * wrapper for zerovm "TrapRead"
 * affects zvm_errno. can be used to detect EOF
 */
int32_t zvm_pread(int desc, char *buffer, int32_t size, int64_t offset)
{
  int32_t code;
  uint64_t request[] = {TrapRead, 0, desc, (uint32_t)buffer, size, offset};
  code = _trap(request);

  /* error encountered */
  if(code < 0)
  {
    zvm_errno_num = -code;
    return -1;
  }

  /* if eof reached (and only if so) user will get 0 */
  return code;
}

/* wrapper for zerovm "TrapWrite". affects zvm_errno */
int32_t zvm_pwrite(int desc, const char *buffer, int32_t size, int64_t offset)
{
  int32_t code;
  uint64_t request[] = {TrapWrite, 0, desc, (uint32_t)buffer, size, offset};
  code = _trap(request);
  if(code < 0)
  {
    zvm_errno_num = -code;
    return -1;
  }
  return code;
}

/* wrapper for zerovm "TrapExit" */
int32_t zvm_exit(int32_t code)
{
  uint64_t request[] = {TrapExit, 0, code};
  return _trap(request);
}

/*
 * set syscallback address. return active syscallback
 * hint: to get syscallback w/o redefining it any invalid pointer
 * could be used, for example 1
 */
int32_t zvm_syscallback(intptr_t addr)
{
  uint64_t request[] = {TrapSyscallback, 0, addr};
  return _trap(request);
}

#undef USER_SIDE
