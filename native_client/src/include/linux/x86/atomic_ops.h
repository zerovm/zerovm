/*
 * Copyright 2008, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef NATIVE_CLIENT_SRC_INCLUDE_LINUX_ATOMIC_OPS_LINUX_H_
#define NATIVE_CLIENT_SRC_INCLUDE_LINUX_ATOMIC_OPS_LINUX_H_ 1

#include "native_client/src/include/portability.h"
#include <stdint.h>

typedef int32_t Atomic32;

static INLINE Atomic32 CompareAndSwap(volatile Atomic32* ptr,
                                      Atomic32 old_value,
                                      Atomic32 new_value) {
  Atomic32 prev;
  __asm__ __volatile__("lock; cmpxchgl %1,%2"
                       : "=a" (prev)
                       : "q" (new_value), "m" (*ptr), "0" (old_value)
                       : "memory");
  return prev;
}


static INLINE Atomic32 AtomicExchange(volatile Atomic32* ptr,
                                      Atomic32 new_value) {
  /* NOTE: The lock prefix is implicit for xchg. */
  __asm__ __volatile__("xchgl %1,%0"
                       : "=r" (new_value)
                       : "m" (*ptr), "0" (new_value)
                       : "memory");
  return new_value;  /* Now it's the previous value. */
}

static INLINE Atomic32 AtomicIncrement(volatile Atomic32* ptr,
                                       Atomic32 increment) {
  Atomic32 temp = increment;
  __asm__ __volatile__("lock; xaddl %0,%1"
                       : "+r" (temp), "+m" (*ptr)
                       : : "memory");
  /* temp now contains the previous value of *ptr */
  return temp + increment;
}

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_LINUX_ATOMIC_OPS_LINUX_H_ */
