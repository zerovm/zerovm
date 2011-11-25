/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>

#include <map>
#include <string>

#include "native_client/src/trusted/gdb_rsp/abi.h"
#include "native_client/src/trusted/port/platform.h"

using port::IPlatform;

namespace gdb_rsp {

#define MINIDEF(x, name, purpose) { #name, sizeof(x), Abi::purpose, 0, 0 }
#define BPDEF(x) { sizeof(x), x }

static Abi::RegDef RegsX86_64[] = {
  MINIDEF(uint64_t, rax, GENERAL),
  MINIDEF(uint64_t, rbx, GENERAL),
  MINIDEF(uint64_t, rcx, GENERAL),
  MINIDEF(uint64_t, rdx, GENERAL),
  MINIDEF(uint64_t, rsi, GENERAL),
  MINIDEF(uint64_t, rdi, GENERAL),
  MINIDEF(uint64_t, rbp, GENERAL),
  MINIDEF(uint64_t, rsp, GENERAL),
  MINIDEF(uint64_t, r8, GENERAL),
  MINIDEF(uint64_t, r9, GENERAL),
  MINIDEF(uint64_t, r10, GENERAL),
  MINIDEF(uint64_t, r11, GENERAL),
  MINIDEF(uint64_t, r12, GENERAL),
  MINIDEF(uint64_t, r13, GENERAL),
  MINIDEF(uint64_t, r14, GENERAL),
  MINIDEF(uint64_t, r15, GENERAL),
  MINIDEF(uint64_t, rip, INST_PTR),
  MINIDEF(uint32_t, eflags, FLAGS),
  MINIDEF(uint32_t, cs, SEGMENT),
  MINIDEF(uint32_t, ss, SEGMENT),
  MINIDEF(uint32_t, ds, SEGMENT),
  MINIDEF(uint32_t, es, SEGMENT),
  MINIDEF(uint32_t, fs, SEGMENT),
  MINIDEF(uint32_t, gs, SEGMENT),
};

static Abi::RegDef RegsX86_32[] = {
  MINIDEF(uint32_t, eax, GENERAL),
  MINIDEF(uint32_t, ecx, GENERAL),
  MINIDEF(uint32_t, edx, GENERAL),
  MINIDEF(uint32_t, ebx, GENERAL),
  MINIDEF(uint32_t, esp, GENERAL),
  MINIDEF(uint32_t, ebp, GENERAL),
  MINIDEF(uint32_t, esi, GENERAL),
  MINIDEF(uint32_t, edi, GENERAL),
  MINIDEF(uint32_t, eip, INST_PTR),
  MINIDEF(uint32_t, eflags, FLAGS),
  MINIDEF(uint32_t, cs, SEGMENT),
  MINIDEF(uint32_t, ss, SEGMENT),
  MINIDEF(uint32_t, ds, SEGMENT),
  MINIDEF(uint32_t, es, SEGMENT),
  MINIDEF(uint32_t, fs, SEGMENT),
  MINIDEF(uint32_t, gs, SEGMENT),
};

static Abi::RegDef RegsArm[] = {
  MINIDEF(uint32_t, r0, GENERAL),
  MINIDEF(uint32_t, r1, GENERAL),
  MINIDEF(uint32_t, r2, GENERAL),
  MINIDEF(uint32_t, r3, GENERAL),
  MINIDEF(uint32_t, r4, GENERAL),
  MINIDEF(uint32_t, r5, GENERAL),
  MINIDEF(uint32_t, r6, GENERAL),
  MINIDEF(uint32_t, r7, GENERAL),
  MINIDEF(uint32_t, r8, GENERAL),
  MINIDEF(uint32_t, r9, GENERAL),
  MINIDEF(uint32_t, r10, GENERAL),
  MINIDEF(uint32_t, r11, GENERAL),
  MINIDEF(uint32_t, r12, GENERAL),
  MINIDEF(uint32_t, sp, STACK_PTR),
  MINIDEF(uint32_t, lr, LINK_PTR),
  MINIDEF(uint32_t, pc, INST_PTR),
};

/*
 * TODO(mcgrathr): This is hlt rather than the canonical int3.  This is a
 * workaround for http://code.google.com/p/nativeclient/issues/detail?id=1730.
 * The only actual use of this is in the temporary breakpoint inserted
 * implicitly at the entry point to catch startup.  The stub code doesn't
 * really keep track of the fact that the trap is its own expected
 * breakpoint hit, so it has no place that knows to adjust for it having
 * it.  When the int3 trap hits, the PC is left after the int3 instruction,
 * i.e. one byte past the place where the breakpoint was inserted.  (This
 * is the hardware behavior, and at least Linux is known not to fiddle the
 * register state left by the hardware.)  If we remove the breakpoint and
 * resume without adjusting the PC back by one byte, then we are executing
 * one byte ahead of where we should be.  That's either in the middle of an
 * instruction, making it an invalid instruction, or skipping a one-byte
 * instruction, breaking the computation by omitting the first instruction
 * of the program.  Since hlt generates a fault-type exception rather than
 * a trap-type, it will leave the PC at the beginning of the instruction.
 * Since nobody cares what exception we actually got, this works around the
 * problem.
 */
static uint8_t BPCodeX86[] = { 0xf4 };

static Abi::BPDef BPX86 = BPDEF(BPCodeX86);

static AbiMap_t *GetAbis() {
  static AbiMap_t *_abis = new AbiMap_t();
  return _abis;
}

// AbiInit & AbiIsAvailable
//   This pair of functions work together as singleton to
// ensure the module has been correctly initialized.  All
// dependant functions should call AbiIsAvailable to ensure
// the module is ready.
static bool AbiInit() {
  Abi::Register("i386", RegsX86_32, sizeof(RegsX86_32), &BPX86);
  Abi::Register("i386:x86-64", RegsX86_64, sizeof(RegsX86_64), &BPX86);

  // TODO(cbiffle) Figure out how to REALLY detect ARM, and define Breakpoint
  Abi::Register("iwmmxt", RegsArm, sizeof(RegsArm), NULL);

  return true;
}

static bool AbiIsAvailable() {
  static bool initialized_ = AbiInit();
  return initialized_;
}



Abi::Abi() {}
Abi::~Abi() {}

void Abi::Register(const char *name, RegDef *regs,
                   uint32_t bytes, const BPDef *bp) {
  uint32_t offs = 0;
  const uint32_t cnt = bytes / sizeof(RegDef);

  // Build indexes and offsets
  for (uint32_t loop = 0; loop < cnt; loop++) {
    regs[loop].index_ = loop;
    regs[loop].offset_ = offs;
    offs += regs[loop].bytes_;
  }

  Abi *abi = new Abi;

  abi->name_ = name;
  abi->regCnt_ = cnt;
  abi->regDefs_= regs;
  abi->ctxSize_ = offs;
  abi->bpDef_ = bp;

  AbiMap_t *abis = GetAbis();
  (*abis)[name] = abi;
}

const Abi* Abi::Find(const char *name) {
  if (!AbiIsAvailable()) {
    IPlatform::LogError("Failed to initalize ABIs.");
    return NULL;
  }

  AbiMap_t::const_iterator itr = GetAbis()->find(name);
  if (itr == GetAbis()->end()) return NULL;

  return itr->second;
}

const Abi* Abi::Get() {
  static const Abi* abi = NULL;

  if ((NULL == abi) && AbiIsAvailable()) {
#ifdef GDB_RSP_ABI_ARM
    abi = Abi::Find("iwmmxt");
#elif GDB_RSP_ABI_X86_64
    abi = Abi::Find("i386:x86-64");
#elif GDB_RSP_ABI_X86
    abi = Abi::Find("i386");
#else
#error "Unknown CPU architecture."
#endif
  }

  return abi;
}

const char* Abi::GetName() const {
  return name_;
}

const Abi::BPDef *Abi::GetBreakpointDef() const {
  return bpDef_;
}

uint32_t Abi::GetContextSize() const {
  return ctxSize_;
}

uint32_t Abi::GetRegisterCount() const {
  return regCnt_;
}

const Abi::RegDef *Abi::GetRegisterDef(uint32_t index) const {
  if (index >= regCnt_) return NULL;

  return &regDefs_[index];
}

const Abi::RegDef *Abi::GetRegisterType(RegType rtype, uint32_t nth) const {
  uint32_t typeNum = 0;

  // Scan for the "nth" register of rtype;
  for (uint32_t regNum = 0; regNum < regCnt_; regNum++) {
    if (rtype == regDefs_[regNum].type_) {
      if (typeNum == nth) return &regDefs_[regNum];
      typeNum++;
    }
  }

  // Otherwise we failed to find it
  return NULL;
}

}  // namespace gdb_rsp
