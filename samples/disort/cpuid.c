/*
 * cpuid.c
 *
 *  Created on: 29.05.2012
 *      Author: YaroslavLitvinov
 */

#include <stdint.h>
#include "cpuid.h"

#define CPUID_ECX_SSE41      0x00080000  /* SSE4.1 Intel */
#define CPUID_ECX_SSE42      0x00100000  /* SSE4.2 Intel*/
#define CPUID_ECX_SSE4A      0x00000040  /* SSE4.A Amd */



static void asm_CPUID(uint32_t op, volatile uint32_t reg[4]) {
 __asm__ volatile("push %%rbx       \n\t" /* save %ebx */
                   "cpuid            \n\t"
                   "movl %%ebx, %1   \n\t"
                   /* save what cpuid just put in %ebx */
                   "pop %%rbx        \n\t"
                   : "=a"(reg[0]), "=S"(reg[1]), "=c"(reg[2]), "=d"(reg[3])
                   : "a"(op)
                   : "cc");
}

static int test_intel_CPU(uint32_t test){
	uint32_t reg[4] = {0, 0, 0, 0 };
	asm_CPUID(1, reg);
	return 0 != (reg[2] & test);
}

static int test_amd_CPU(uint32_t test){
	uint32_t reg[4] = {0, 0, 0, 0 };
	asm_CPUID(0x80000001, reg);
	return 0 != (reg[2] & test);
}



int test_sse41_CPU(){
	return test_intel_CPU(CPUID_ECX_SSE41);
}

int test_sse4A_CPU(){
	return test_amd_CPU(CPUID_ECX_SSE4A);
}
