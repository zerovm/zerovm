/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/validator/x86/nacl_cpuid.h"

/*
 * nacl_cpuid.c
 * Retrieve and decode CPU model specific feature mask.
 */
#if NACL_WINDOWS
#include <intrin.h>  /* __cpuid intrinsic */
#endif  /* NACL_WINDOWS  */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "native_client/src/include/portability_io.h"

/*
 * TODO(bradchen): consolidate to use one debug print mechanism.
 */

/*  The list of features we can get from the CPUID. */
typedef enum {
  CPUFeature_x87 = 0,
  CPUFeature_MMX,
  CPUFeature_SSE,
  CPUFeature_SSE2,
  CPUFeature_SSE3,
  CPUFeature_SSSE3,
  CPUFeature_SSE41,
  CPUFeature_SSE42,
  CPUFeature_MOVBE,
  CPUFeature_POPCNT,
  CPUFeature_CX8,
  CPUFeature_CX16,
  CPUFeature_CMOV,
  CPUFeature_MON,
  CPUFeature_FXSR,
  CPUFeature_CLFLUSH,
  /* These instructions are illegal but included for completeness */
  CPUFeature_MSR,
  CPUFeature_TSC,
  CPUFeature_VME,
  CPUFeature_PSN,
  CPUFeature_VMX,
  CPUFeature_OSXSAVE,
  CPUFeature_AVX,
  /* AMD-specific features */
  CPUFeature_3DNOW,
  CPUFeature_EMMX,
  CPUFeature_E3DNOW,
  CPUFeature_LZCNT,
  CPUFeature_SSE4A,
  CPUFeature_LM,
  CPUFeature_SVM,
  CPUFeature_Last
} CPUFeatureID;

#define CPUID_EDX_x87        0x00000001  /* x87 FPU support */
#define CPUID_EDX_VME        0x00000002  /* Virtual 8086 Mode Enhancement */
#define CPUID_EDX_DEB        0x00000004  /* Debugging Extensions */
#define CPUID_EDX_PSE        0x00000008  /* Page Size Extensions */
#define CPUID_EDX_TSC        0x00000010  /* Time Stamp Counter */
#define CPUID_EDX_MSR        0x00000020  /* RDMSR and WRMSR */
#define CPUID_EDX_PAE        0x00000040  /* Physical Address Extensions */
#define CPUID_EDX_MCE        0x00000080  /* Machine Check Exception */
#define CPUID_EDX_CX8        0x00000100  /* CMPXCHG8B Instruction */
#define CPUID_EDX_APIC       0x00000200  /* APIC on chip */
/* 0x00000400 reserved */
#define CPUID_EDX_SEP        0x00000800  /* SYSENTER and SYSEXIT */
#define CPUID_EDX_MTRR       0x00001000  /* Memory Type Range Registers */
#define CPUID_EDX_PGE        0x00002000  /* PTE Global Bit */
#define CPUID_EDX_MCA        0x00004000  /* Machine Check Architecture */
#define CPUID_EDX_CMOV       0x00008000  /* CMOV instruction */
#define CPUID_EDX_PAT        0x00010000  /* Page Attribute Table */
#define CPUID_EDX_PSE36      0x00020000  /* Page Size Extension bis */
#define CPUID_EDX_PSN        0x00040000  /* Processor Serial Number */
#define CPUID_EDX_CLFLUSH    0x00080000  /* CLFLUSH instruction */
/* 0x00100000 reserved */
#define CPUID_EDX_DS         0x00200000  /* Debug Store */
#define CPUID_EDX_ACPI       0x00400000  /* Thermal Monitor and Clock Ctrl */
#define CPUID_EDX_MMX        0x00800000  /* MMX extensions */
#define CPUID_EDX_FXSR       0x01000000  /* FXSAVE/FXRSTOR instructions */
#define CPUID_EDX_SSE        0x02000000  /* SSE extensions */
#define CPUID_EDX_SSE2       0x04000000  /* SSE2 extensions */
#define CPUID_EDX_SS         0x08000000  /* self snoop */
#define CPUID_EDX_HTT        0x10000000  /* hyper-threading */
#define CPUID_EDX_TM         0x20000000  /* Thermal monitor */
/* 0x40000000 reserved */
#define CPUID_EDX_PBE        0x80000000  /* Pending Break Enable */

#define CPUID_ECX_SSE3       0x00000001  /* SSE3 extensions */
/* 0x00000002 reserved */
/* 0x00000004 reserved */
#define CPUID_ECX_MON        0x00000008  /* MONITOR/MWAIT instructions */
#define CPUID_ECX_DSCPL      0x00000010  /* CPL Qualified Debug Store */
#define CPUID_ECX_VMX        0x00000020  /* Virtual Machine Extensions */
#define CPUID_ECX_SMX        0x00000040  /* Safer Mode Extensions */
#define CPUID_ECX_EST        0x00000080  /* Enahcned SpeedStep */
#define CPUID_ECX_TM2        0x00000100  /* Thermal Monitor 2 */
#define CPUID_ECX_SSSE3      0x00000200  /* SS_S_E3 extensions */
#define CPUID_ECX_CXID       0x00000400  /* L1 context ID */
/* 0x00000800 reserved */
/* 0x00001000 reserved */
#define CPUID_ECX_CX16       0x00002000  /* CMPXCHG16B instruction */
#define CPUID_ECX_XTPR       0x00004000  /* xTPR update control */
#define CPUID_ECX_PDCM       0x00008000  /* Perf/Debug Capability MSR */
/* 0x00010000 reserved */
/* 0x00020000 reserved */
/* 0x00040000 reserved */
#define CPUID_ECX_SSE41      0x00080000  /* SSE4.1 extensions */
#define CPUID_ECX_SSE42      0x00100000  /* SSE4.2 extensions */
/* 0x00200000 reserved */
#define CPUID_ECX_MOVBE      0x00400000  /* MOVBE instruction */
#define CPUID_ECX_POPCNT     0x00800000  /* POPCNT instruction */
/* 0x00100000 reserved */
#define CPUID_ECX_AES        0x02000000  /* AES instructions */
#define CPUID_ECX_XSAVE      0x04000000  /* XSAVE/XRSTOR/XSETBV/XGETBV */
#define CPUID_ECX_OSXSAVE    0x08000000  /* XSAVE et al enabled by OS */
#define CPUID_ECX_AVX        0x10000000  /* AVX instructions */
/* 0x20000000 reserved */
/* 0x40000000 reserved */
/* 0x80000000 reserved */

/* AMD-specific masks */
#define CPUID_EDX_EMMX       0x00400000
#define CPUID_EDX_LM         0x20000000  /* longmode */
#define CPUID_EDX_E3DN       0x40000000
#define CPUID_EDX_3DN        0x80000000
#define CPUID_ECX_SVM        0x00000004
#define CPUID_ECX_ABM        0x00000020  /* lzcnt instruction */
#define CPUID_ECX_SSE4A      0x00000040
#define CPUID_ECX_PRE        0x00000100
#define CPUID_ECX_SSE5       0x00000800

typedef enum {
  CFReg_EAX_I=0,  /* eax == 1 */
  CFReg_EBX_I,    /* eax == 1 */
  CFReg_ECX_I,    /* eax == 1 */
  CFReg_EDX_I,    /* eax == 1 */
  CFReg_EAX_A,    /* eax == 0x80000001 */
  CFReg_EBX_A,    /* eax == 0x80000001 */
  CFReg_ECX_A,    /* eax == 0x80000001 */
  CFReg_EDX_A     /* eax == 0x80000001 */
} CPUFeatureReg;

typedef struct cpufeature {
  CPUFeatureReg reg;
  uint32_t mask;
  const char *name;
} CPUFeature;

static const CPUFeature CPUFeatureDescriptions[(int)CPUFeature_Last] = {
  {CFReg_EDX_I, CPUID_EDX_x87, "x87 FPU"},
  {CFReg_EDX_I, CPUID_EDX_MMX, "MMX"},
  {CFReg_EDX_I, CPUID_EDX_SSE, "SSE"},
  {CFReg_EDX_I, CPUID_EDX_SSE2, "SSE2"},
  {CFReg_ECX_I, CPUID_ECX_SSE3, "SSE3"},
  {CFReg_ECX_I, CPUID_ECX_SSSE3, "SSSE3"},
  {CFReg_ECX_I, CPUID_ECX_SSE41, "SSE41"},
  {CFReg_ECX_I, CPUID_ECX_SSE42, "SSE42"},
  {CFReg_ECX_I, CPUID_ECX_MOVBE, "MOVBE"},
  {CFReg_ECX_I, CPUID_ECX_POPCNT, "POPCNT"},
  {CFReg_EDX_I, CPUID_EDX_CX8, "CMPXCHG8B"},
  {CFReg_ECX_I, CPUID_ECX_CX16, "CMPXCHG16B"},
  {CFReg_EDX_I, CPUID_EDX_CMOV, "CMOV"},
  {CFReg_ECX_I, CPUID_ECX_MON, "MONITOR/MWAIT"},
  {CFReg_EDX_I, CPUID_EDX_FXSR, "FXSAVE/FXRSTOR"},
  {CFReg_EDX_I, CPUID_EDX_CLFLUSH, "CLFLUSH"},
  {CFReg_EDX_I, CPUID_EDX_MSR, "RDMSR/WRMSR"},
  {CFReg_EDX_I, CPUID_EDX_TSC, "RDTSC"},
  {CFReg_EDX_I, CPUID_EDX_VME, "VME"},
  {CFReg_EDX_I, CPUID_EDX_PSN, "PSN"},
  {CFReg_ECX_I, CPUID_ECX_VMX, "VMX"},
  {CFReg_ECX_I, CPUID_ECX_OSXSAVE, "OSXSAVE"},
  {CFReg_ECX_I, CPUID_ECX_AVX, "AVX"},
  {CFReg_EDX_A, CPUID_EDX_3DN, "3DNow"},
  {CFReg_EDX_A, CPUID_EDX_EMMX, "EMMX"},
  {CFReg_EDX_A, CPUID_EDX_E3DN, "E3DNow"},
  {CFReg_ECX_A, CPUID_ECX_ABM, "LZCNT"},
  {CFReg_ECX_A, CPUID_ECX_SSE4A, "SSE4A"},
  {CFReg_EDX_A, CPUID_EDX_LM, "LongMode"},
  {CFReg_ECX_A, CPUID_ECX_SVM, "SVM"},
};

#define /* static const int */ kVendorIDLength 13
static const char Intel_CPUID0[kVendorIDLength]   = "GenuineIntel";
static const char AMD_CPUID0[kVendorIDLength]     = "AuthenticAMD";
#ifdef NOTYET
static const char UMC_CPUID0[kVendorIDLength]     = "UMC UMC UMC ";
static const char Cyrix_CPUID0[kVendorIDLength]   = "CyrixInstead";
static const char NexGen_CPUID0[kVendorIDLength]  = "NexGenDriven";
static const char Cantaur_CPUID0[kVendorIDLength] = "CentaurHauls";
static const char Rise_CPUID0[kVendorIDLength]    = "RiseRiseRise";
static const char SiS_CPUID0[kVendorIDLength]     = "SiS SiS SiS ";
static const char TM_CPUID0[kVendorIDLength]      = "GenuineTMx86";
static const char NSC_CPUID0[kVendorIDLength]     = "Geode by NSC";
#endif

static int asm_HasCPUID() {
  volatile int before, after, result;
#if NACL_BUILD_SUBARCH == 64
  /* Note: If we are running in x86-64, then cpuid must be defined,
   * since CPUID dates from DX2 486, and x86-64 was added after this.
   */
  return 1;
/* TODO(bradchen): split into separate Windows, etc., files */
#elif defined(__GNUC__)
  __asm__ volatile("pushfl                \n\t" /* save EFLAGS to eax */
                   "pop %%eax             \n\t"
                   "movl %%eax, %0        \n\t" /* remember EFLAGS in %0 */
                   "xor $0x00200000, %%eax\n\t" /* toggle bit 21 */
                   "push %%eax            \n\t" /* write eax to EFLAGS */
                   "popfl                 \n\t"
                   "pushfl                \n\t" /* save EFLAGS to %1 */
                   "pop %1                \n\t"
                   /*
                    * We use "r" constraints here, forcing registers,
                    * because a memory reference using the stack
                    * pointer wouldn't be safe since we're moving the
                    * stack pointer around in between the
                    * instructions.  We need to inform the compiler
                    * that we're clobbering %eax as a scratch register.
                    */
                   : "=r" (before), "=r" (after) : : "eax");
#elif NACL_WINDOWS
  __asm {
    pushfd
    pop eax
    mov before, eax
    xor eax, 0x00200000
    push eax
    popfd
    pushfd
    pop after
  }
#else
# error Unsupported platform
#endif
  result = (before ^ after) & 0x0200000;
  return result;
}

static void asm_CPUID(uint32_t op, volatile uint32_t reg[4]) {
#if defined(__GNUC__)
#if NACL_BUILD_SUBARCH == 64
 __asm__ volatile("push %%rbx       \n\t" /* save %ebx */
#else
 __asm__ volatile("pushl %%ebx      \n\t"
#endif
                   "cpuid            \n\t"
                   "movl %%ebx, %1   \n\t"
                   /* save what cpuid just put in %ebx */
#if NACL_BUILD_SUBARCH == 64
                   "pop %%rbx        \n\t"
#else
                   "popl %%ebx       \n\t" /* restore the old %ebx */
#endif
                   : "=a"(reg[0]), "=S"(reg[1]), "=c"(reg[2]), "=d"(reg[3])
                   : "a"(op)
                   : "cc");
#elif NACL_WINDOWS
  __cpuid((uint32_t*)reg, op);
#else
# error Unsupported platform
#endif
}

static void CacheCPUVersionID(NaClCPUData* data) {
  uint32_t reg[4] = {0, 0, 0, 0 };
  asm_CPUID(0, reg);
  data->_vidwords[0] = reg[1];
  data->_vidwords[1] = reg[3];
  data->_vidwords[2] = reg[2];
  data->_vidwords[3] = 0;
}

/* Defines the (cached) cpu version id */
#define CPUVersionID(data) ((char*) (data)->_vidwords)


/* Cache feature vector as array of uint32_t [ecx, edx] */
static void CacheCPUFeatureVector(NaClCPUData* data) {
  int i;
  for (i = 0; i < kMaxCPUFeatureReg; ++i) {
    data->_featurev[i] = 0;
  }
  asm_CPUID(1, data->_featurev);
  /* this is for AMD CPUs */
  asm_CPUID(0x80000001, &data->_featurev[CFReg_EAX_A]);
#if 0
  /* print feature vector */
  printf("CPUID:  %08x  %08x  %08x  %08x\n",
         data->_featurev[0],
         data->_featurev[1],
         data->_featurev[2],
         data->_featurev[3]);
  printf("CPUID:  %08x  %08x  %08x  %08x\n",
         data->_featurev[4],
         data->_featurev[5],
         data->_featurev[6],
         data->_featurev[7]);
#endif
}

/* CacheGetCPUIDString creates an ASCII string that identfies this CPU's */
/* vendor ID, family, model, and stepping, as per the CPUID instruction */
static void CacheGetCPUIDString(NaClCPUData* data) {
  char *cpuversionid = CPUVersionID(data);
  uint32_t *fv = data->_featurev;
  char* wlid = data->_wlid;
  /* Subtract 1 in this assert to avoid counting two null characters. */
  assert(9 + kVendorIDLength - 1 == kCPUIDStringLength);
  memcpy(wlid, cpuversionid, kVendorIDLength-1);
  SNPRINTF(&(wlid[kVendorIDLength-1]), 9, "%08x", (int)fv[CFReg_EAX_I]);
}

char *GetCPUIDString(NaClCPUData* data) {
  return data->_wlid;
}

/* Returns true if the given feature is defined by the CPUID. */
static Bool CheckCPUFeature(NaClCPUData* data, CPUFeatureID fid) {
  const CPUFeature *f = &CPUFeatureDescriptions[fid];
  uint32_t *fv = data->_featurev;
#if 0
  printf("%s: %x (%08x & %08x)\n", f->name, (fv[f->reg] & f->mask),
         fv[f->reg], f->mask);
#endif
  if (fv[f->reg] & f->mask) {
    return TRUE;
  } else {
    return FALSE;
  }
}

uint64_t NaClXGETBV(uint32_t);

/* Cache XCR vector */
static void CacheCPUXCRVector(NaClCPUData* data) {
  if (CheckCPUFeature(data, CPUFeature_OSXSAVE)) {
    int i;
    for (i = 0; i < kMaxCPUXCRReg; ++i) {
      data->_xcrv[i] = NaClXGETBV(i);
    }
  } else {
    int i;
    for (i = 0; i < kMaxCPUXCRReg; ++i) {
      data->_xcrv[i] = 0;
    }
  }
}

/* Check that we have a supported 386 architecture. NOTE:
 * As as side effect, the given cpu features is cleared before
 * setting the appropriate fields.
 */
static void CheckNaClArchFeatures(NaClCPUData* data,
                                  nacl_arch_features* features) {
  const size_t kCPUID0Length = 12;
  char *cpuversionid;
  memset(features, 0, sizeof(*features));
  if (data->_has_CPUID) features->f_cpuid_supported = 1;
  cpuversionid = CPUVersionID(data);
  if (strncmp(cpuversionid, Intel_CPUID0, kCPUID0Length) == 0) {
    features->f_cpu_supported = 1;
  } else if (strncmp(cpuversionid, AMD_CPUID0, kCPUID0Length) == 0) {
    features->f_cpu_supported = 1;
  }
}

Bool NaClArchSupported(NaClCPUData* data) {
  nacl_arch_features features;
  CheckNaClArchFeatures(data, &features);
  return (Bool) (features.f_cpuid_supported && features.f_cpu_supported);
}

void NaClSetAllCPUFeatures(CPUFeatures *features) {
  /* Since CPUFeatures is a struct that contains only Bools, we could
   * use any non-zero value here, but 0xff seems safest.
   */
  memset(features, 0xff, sizeof(*features));
}

void NaClClearCPUFeatures(CPUFeatures *features) {
  memset(features, 0, sizeof(*features));
}

void NaClCopyCPUFeatures(CPUFeatures* target, const CPUFeatures* source) {
  memcpy(target, source, sizeof(CPUFeatures));
}

/* WARNING: This routine and subroutines it uses are not threadsafe.
 * However, if races occur, they are short lived, and at worst, will
 * result in defining fewer features than are actually supported by
 * the hardware. Hence, if a race occurs, the validator may reject
 * some features that should not be rejected.
 */
void GetCPUFeatures(NaClCPUData* data, CPUFeatures *cpuf) {
  NaClClearCPUFeatures(cpuf);
  CheckNaClArchFeatures(data, &cpuf->arch_features);
  if (!cpuf->arch_features.f_cpuid_supported) return;
  cpuf->f_x87 = CheckCPUFeature(data, CPUFeature_x87);
  cpuf->f_MMX = CheckCPUFeature(data, CPUFeature_MMX);
  cpuf->f_SSE = CheckCPUFeature(data, CPUFeature_SSE);
  cpuf->f_SSE2 = CheckCPUFeature(data, CPUFeature_SSE2);
  cpuf->f_SSE3 = CheckCPUFeature(data, CPUFeature_SSE3);
  cpuf->f_SSSE3 = CheckCPUFeature(data, CPUFeature_SSSE3);
  cpuf->f_SSE41 = CheckCPUFeature(data, CPUFeature_SSE41);
  cpuf->f_SSE42 = CheckCPUFeature(data, CPUFeature_SSE42);
  cpuf->f_MOVBE = CheckCPUFeature(data, CPUFeature_MOVBE);
  cpuf->f_POPCNT = CheckCPUFeature(data, CPUFeature_POPCNT);
  cpuf->f_CX8 = CheckCPUFeature(data, CPUFeature_CX8);
  cpuf->f_CX16 = CheckCPUFeature(data, CPUFeature_CX16);
  cpuf->f_CMOV = CheckCPUFeature(data, CPUFeature_CMOV);
  cpuf->f_MON = CheckCPUFeature(data, CPUFeature_MON);
  cpuf->f_FXSR = CheckCPUFeature(data, CPUFeature_FXSR);
  cpuf->f_CLFLUSH = CheckCPUFeature(data, CPUFeature_CLFLUSH);
  /* These instructions are illegal but included for completeness */
  cpuf->f_MSR = CheckCPUFeature(data, CPUFeature_MSR);
  cpuf->f_TSC = CheckCPUFeature(data, CPUFeature_TSC);
  cpuf->f_VME = CheckCPUFeature(data, CPUFeature_VME);
  cpuf->f_PSN = CheckCPUFeature(data, CPUFeature_PSN);
  cpuf->f_VMX = CheckCPUFeature(data, CPUFeature_VMX);
  cpuf->f_OSXSAVE = CheckCPUFeature(data, CPUFeature_OSXSAVE);
  /* AMD-specific features */
  cpuf->f_3DNOW = CheckCPUFeature(data, CPUFeature_3DNOW);
  cpuf->f_EMMX = CheckCPUFeature(data, CPUFeature_EMMX);
  cpuf->f_E3DNOW = CheckCPUFeature(data, CPUFeature_E3DNOW);
  cpuf->f_LZCNT = CheckCPUFeature(data, CPUFeature_LZCNT);
  cpuf->f_SSE4A = CheckCPUFeature(data, CPUFeature_SSE4A);
  cpuf->f_LM = CheckCPUFeature(data, CPUFeature_LM);
  cpuf->f_SVM = CheckCPUFeature(data, CPUFeature_SVM);

  /*
   * If the operating system doesn't maintain the AVX state,
   * pretend we don't have the instructions available at all.
   */
  cpuf->f_AVX = (CheckCPUFeature(data, CPUFeature_AVX) &&
                 cpuf->f_OSXSAVE &&
                 (data->_xcrv[0] & 6) == 6);
}

void NaClCPUDataGet(NaClCPUData* data) {
  data->_has_CPUID = asm_HasCPUID();
  CacheCPUVersionID(data);
  CacheCPUVersionID(data);
  CacheCPUFeatureVector(data);
  CacheCPUXCRVector(data);
  CacheGetCPUIDString(data);
}
