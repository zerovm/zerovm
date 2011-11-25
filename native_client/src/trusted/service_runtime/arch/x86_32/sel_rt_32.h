/*
 * Copyright 2009 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Secure Runtime
 */

#ifndef __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_32_SEL_RT_32_H__
#define __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_32_SEL_RT_32_H__ 1

#include "native_client/src/include/portability.h"

uint16_t NaClGetGlobalCs(void);

uint16_t NaClGetGlobalDs(void);

uint16_t NaClGetCs(void);
/* setting CS is done using an lcall */

uint16_t NaClGetDs(void);

void    NaClSetDs(uint16_t v);

uint16_t NaClGetEs(void);

void    NaClSetEs(uint16_t v);

uint16_t NaClGetFs(void);

void    NaClSetFs(uint16_t v);

uint16_t NaClGetGs(void);

void    NaClSetGs(uint16_t v);

uint16_t NaClGetSs(void);

/* not really a segment registers, but ... */

uint32_t NaClGetEbx(void);

/*
 * On a context switch through the syscall interface, not all
 * registers are saved.  We assume that C calling convention is used,
 * so %ecx and %edx are caller-saved and the NaCl service runtime does
 * not have to bother saving them; %eax (or %edx:%eax pair) should
 * have the return value, so its old value is also not saved.  (We
 * should, however, ensure that there is not an accidental covert
 * channel leaking information via these registers on syscall return.)
 * The eflags register is also caller saved.
 *
 * We assume that the following is packed.  This is true for gcc and
 * msvc for x86, but we will include a check that sizeof(struct
 * NaClThreadContext) == 64 bytes. (32-bit and 64-bit mode)
 */

typedef uint32_t  nacl_reg_t;  /* general purpose register type */

#define NACL_PRIdNACL_REG NACL_PRId32
#define NACL_PRIiNACL_REG NACL_PRIi32
#define NACL_PRIoNACL_REG NACL_PRIo32
#define NACL_PRIuNACL_REG NACL_PRIu32
#define NACL_PRIxNACL_REG NACL_PRIx32
#define NACL_PRIXNACL_REG NACL_PRIX32

/*
 * TODO(bsy): remove this.  We no longer support 64/32 split mode
 * execution where the service runtime is 64-bit code but the NaCl
 * module is 32-bit code, and making the registers explicit/concrete
 * can be nice.
 */
/* 8-bytes */
union PtrAbstraction {
  uint64_t ptr_64;
  struct {
    uint32_t ptr_padding;
    uint32_t ptr;
  } ptr_32;
};

struct NaClThreadContext {
  /* ecx, edx, eax, eflags not saved */
  nacl_reg_t  ebx, esi, edi, prog_ctr; /* return addr */
  /*          0    4    8    c */
  union PtrAbstraction frame_ptr;
  /*          10 */
  union PtrAbstraction stack_ptr;
  /*          18 */
  uint16_t    ss; /* stack_ptr and ss must be adjacent */
  /*          20 */
  char        dummy[6];
  /*
   * gs is our TLS base in the app; on the host side it's either fs or gs.
   */
  uint16_t    ds, es, fs, gs;
  /*          28  2a  2c  2e */
  /*
   * spring_addr, sys_ret and new_eip are not a part of the thread's
   * register set, but are needed by NaClSwitch. By including them
   * here, the two use the same interface.
   */
  nacl_reg_t  new_prog_ctr;
  /*          30 */
  nacl_reg_t  sysret;
  /*          34 */
  nacl_reg_t  spring_addr;
  /*          38 */
  uint16_t    cs; /* spring_addr and cs must be adjacent */
  /*          3c */
};

#endif /* __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_32_SEL_RT_32_H__ */
