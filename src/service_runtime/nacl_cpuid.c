/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/service_runtime/nacl_cpuid.h"

/*
 * nacl_cpuid.c
 * Retrieve and decode CPU model specific feature mask.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "include/portability_io.h"
#include "src/platform/nacl_log.h"

/*
 * TODO(bradchen): consolidate to use one debug print mechanism.
 */

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

static const CPUFeature CPUFeatureDescriptions[(int)NaClCPUFeature_Max] = {
  {CFReg_EDX_I, CPUID_EDX_x87, "x87"},
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
static int CheckCPUFeature(NaClCPUData* data, NaClCPUFeatureID fid) {
  const CPUFeature *f = &CPUFeatureDescriptions[fid];
  uint32_t *fv = data->_featurev;
#if 0
  printf("%s: %x (%08x & %08x)\n", f->name, (fv[f->reg] & f->mask),
         fv[f->reg], f->mask);
#endif
  if (fv[f->reg] & f->mask) {
    return 1;
  } else {
    return 0;
  }
}

uint64_t NaClXGETBV(uint32_t);

/* Cache XCR vector */
static void CacheCPUXCRVector(NaClCPUData* data) {
  if (CheckCPUFeature(data, NaClCPUFeature_OSXSAVE)) {
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

int NaClArchSupported(const NaClCPUFeaturesX86 *features) {
  return (features->arch_features.f_cpuid_supported &&
          features->arch_features.f_cpu_supported);
}

void NaClClearCPUFeatures(NaClCPUFeaturesX86 *features) {
  memset(features, 0, sizeof(*features));
}

void NaClSetAllCPUFeatures(NaClCPUFeaturesX86 *features) {
  /* Be a little more pedantic than using memset because we don't know exactly
   * how the structure is laid out.  If we use memset, fields may be initialized
   * to 0xff instead of 1 ... this isn't the end of the world but it can
   * create a skew if the structure is hashed, etc.
   */
  int id;
  /* Ensure any padding is zeroed. */
  NaClClearCPUFeatures(features);
  features->arch_features.f_cpuid_supported = 1;
  features->arch_features.f_cpu_supported = 1;
  for (id = 0; id < NaClCPUFeature_Max; ++id) {
    NaClSetCPUFeature(features, id, 1);
  }
}

void NaClCopyCPUFeatures(NaClCPUFeaturesX86* target,
                         const NaClCPUFeaturesX86* source) {
  memcpy(target, source, sizeof(NaClCPUFeaturesX86));
}

void NaClSetCPUFeature(NaClCPUFeaturesX86 *features, NaClCPUFeatureID id,
                       int state) {
  features->data[id] = (char) state;
}

const char* NaClGetCPUFeatureName(NaClCPUFeatureID id) {
  return CPUFeatureDescriptions[id].name;
}

/* WARNING: This routine and subroutines it uses are not threadsafe.
 * However, if races occur, they are short lived, and at worst, will
 * result in defining fewer features than are actually supported by
 * the hardware. Hence, if a race occurs, the validator may reject
 * some features that should not be rejected.
 */
static void GetCPUFeatures(NaClCPUData* data, NaClCPUFeaturesX86 *cpuf) {
  int id;
  NaClClearCPUFeatures(cpuf);
  CheckNaClArchFeatures(data, &cpuf->arch_features);
  if (!cpuf->arch_features.f_cpuid_supported) return;

  for (id = 0; id < NaClCPUFeature_Max; ++id) {
    NaClSetCPUFeature(cpuf, id, CheckCPUFeature(data, id));
  }

  /*
   * If the operating system doesn't maintain the AVX state,
   * pretend we don't have the instructions available at all.
   */
  if (!(NaClGetCPUFeature(cpuf, NaClCPUFeature_OSXSAVE)
        && (data->_xcrv[0] & 6) == 6)) {
    NaClSetCPUFeature(cpuf, NaClCPUFeature_AVX, 0);
  }
}

void NaClCPUDataGet(NaClCPUData* data) {
  data->_has_CPUID = asm_HasCPUID();
  CacheCPUVersionID(data);
  CacheCPUFeatureVector(data);
  CacheCPUXCRVector(data);
  CacheGetCPUIDString(data);
}

void NaClGetCurrentCPUFeatures(NaClCPUFeaturesX86 *cpu_features) {
  NaClCPUData cpu_data;
  NaClCPUDataGet(&cpu_data);
  GetCPUFeatures(&cpu_data, cpu_features);
}

/* This array defines the CPU feature model for fixed-feature CPU
 * mode. We currently require the same set of features for both
 * 32- and 64-bit x86 CPUs, intended to be supported by most/all
 * post-Pentium III CPUs. This set may be something we need to
 * revisit in the future.
 */
const int kFixedFeatureCPUModel[NaClCPUFeature_Max] = {
  0, /* NaClCPUFeature_x87 */
  0, /* NaClCPUFeature_MMX */
  1, /* NaClCPUFeature_SSE */
  1, /* NaClCPUFeature_SSE2 */
  1, /* NaClCPUFeature_SSE3 */
  0, /* NaClCPUFeature_SSSE3 */
  0, /* NaClCPUFeature_SSE41 */
  0, /* NaClCPUFeature_SSE42 */
  0, /* NaClCPUFeature_MOVBE */
  0, /* NaClCPUFeature_POPCNT */
  1, /* NaClCPUFeature_CX8 */
  1, /* NaClCPUFeature_CX16 */
  1, /* NaClCPUFeature_CMOV */
  0, /* NaClCPUFeature_MON */
  1, /* NaClCPUFeature_FXSR */
  1, /* NaClCPUFeature_CLFLUSH */
  0, /* NaClCPUFeature_MSR */
  1, /* NaClCPUFeature_TSC */
  0, /* NaClCPUFeature_VME */
  0, /* NaClCPUFeature_PSN */
  0, /* NaClCPUFeature_VMX */
  0, /* NaClCPUFeature_OSXSAVE */
  0, /* NaClCPUFeature_AVX */
  0, /* NaClCPUFeature_3DNOW */  /* AMD-specific */
  0, /* NaClCPUFeature_EMMX */   /* AMD-specific */
  0, /* NaClCPUFeature_E3DNOW */ /* AMD-specific */
  0, /* NaClCPUFeature_LZCNT */  /* AMD-specific */
  0, /* NaClCPUFeature_SSE4A */  /* AMD-specific */
  0, /* NaClCPUFeature_LM */
  0, /* NaClCPUFeature_SVM */    /* AMD-specific */
};

int NaClFixCPUFeatures(NaClCPUFeaturesX86 *cpu_features) {
  NaClCPUFeatureID fid;
  int rvalue = 1;

  for (fid = 0; fid < NaClCPUFeature_Max; fid++) {
    if (kFixedFeatureCPUModel[fid]) {
      if (!NaClGetCPUFeature(cpu_features, fid)) {
        /* This CPU is missing a required feature. */
        NaClLog(LOG_ERROR,
                "This CPU is missing a feature required by fixed-mode: %s\n",
                NaClGetCPUFeatureName(fid));
        rvalue = 0;  /* set return value to indicate failure */
      }
    } else {
      /* Feature is not in the fixed model.
       * Ensure cpu_features does not have it either.
       */
      NaClSetCPUFeature(cpu_features, fid, 0);
    }
  }
  return rvalue;
}
