/*
 * trap.h
 *
 *  Created on: Dec 2, 2011
 *      Author: d'b
 */

#ifndef TRAP_H_
#define TRAP_H_

#include "api/zvm.h"
#include "src/service_runtime/sel_ldr.h"

EXTERN_C_BEGIN

/*
 * our "One Ring" syscall main routine
 * call given syscall with the given parameteres supposed to be invoked from NaClSysNanosleep()
 *
 * 1st parameter is a pointer to the command (function, arg1, argv2,..)
 * 2nd parameter is a pointer to return value(s)
 * return 0 if successful, otherwise - 1 (nanosleep restriction)
 * note: in a future will be moved to "nacl_syscall_handler.c" to be only syscall
 */
int32_t TrapHandler(struct NaClApp *nap, uint32_t args);

/* pause cpu time counting. update cnt_cpu */
void PauseCpuClock(struct NaClApp *nap);

/* resume cpu time counting */
void ResumeCpuClock(struct NaClApp *nap);

EXTERN_C_END

#endif /* TRAP_H_ */
