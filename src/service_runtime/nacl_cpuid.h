/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This module provides a simple abstraction for using the CPUID
 * instruction to determine instruction set extensions supported by
 * the current processor.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NACL_CPUID_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NACL_CPUID_H_

#include "include/portability.h"

EXTERN_C_BEGIN

/* The list of features we can get from the CPUID instruction.
 * Do not modify this enum without making similar modifications to
 * CPUFeatureDescriptions in nacl_cpuid.c.
 */
typedef enum {
  NaClCPUFeature_x87 = 0,
  NaClCPUFeature_MMX,
  NaClCPUFeature_SSE,
  NaClCPUFeature_SSE2,
  NaClCPUFeature_SSE3,
  NaClCPUFeature_SSSE3,
  NaClCPUFeature_SSE41,
  NaClCPUFeature_SSE42,
  NaClCPUFeature_MOVBE,
  NaClCPUFeature_POPCNT,
  NaClCPUFeature_CX8,
  NaClCPUFeature_CX16,
  NaClCPUFeature_CMOV,
  NaClCPUFeature_MON,
  NaClCPUFeature_FXSR,
  NaClCPUFeature_CLFLUSH,
  NaClCPUFeature_MSR,
  NaClCPUFeature_TSC,
  NaClCPUFeature_VME,
  NaClCPUFeature_PSN,
  NaClCPUFeature_VMX,
  NaClCPUFeature_OSXSAVE,
  NaClCPUFeature_AVX,
  NaClCPUFeature_3DNOW, /* AMD-specific */
  NaClCPUFeature_EMMX, /* AMD-specific */
  NaClCPUFeature_E3DNOW, /* AMD-specific */
  NaClCPUFeature_LZCNT, /* AMD-specific */
  NaClCPUFeature_SSE4A, /* AMD-specific */
  NaClCPUFeature_LM,
  NaClCPUFeature_SVM, /* AMD-specific */
  NaClCPUFeature_Max
} NaClCPUFeatureID;

/* Features needed to show that the architecture is supported. */
typedef struct nacl_arch_features {
  char f_cpuid_supported;  /* CPUID is defined for the hardward. */
  char f_cpu_supported;    /* CPU is one we support. */
} nacl_arch_features;

/* Features we can get about the x86 hardware. */
typedef struct cpu_feature_struct {
  nacl_arch_features arch_features;
  char data[NaClCPUFeature_Max];
} NaClCPUFeaturesX86;

typedef NaClCPUFeaturesX86 NaClCPUFeatures; /* d'b */

/* Define the maximum length of a CPUID string.
 *
 * Note: If you change this length, fix the static initialization of wlid
 * in nacl_cpuid.c to be initialized with an appropriate string.
 */
#define /* static const int */ kCPUIDStringLength 21

/* Defines the maximum number of feature registers used to hold CPUID.
 * Note: This value corresponds to the number of enumerated elements in
 * enum CPUFeatureReg defined in nacl_cpuid.c.
 */
#define kMaxCPUFeatureReg 8

/* Defines the maximum number of extended control registers.
 */
#define kMaxCPUXCRReg 1

/* Define a cache for collected CPU runtime information, from which
 * queries can answer questions.
 */
typedef struct NaClCPUData {
  /* The following is used to cache whether CPUID is defined for the
   * architecture the code is running on.
   */
  int _has_CPUID;
  /* Version ID words used by CPUVersionID. */
  uint32_t _vidwords[4];
  /* Define the set of CPUID feature register values for the architecture.
   * Note: We have two sets (of 4 registers) so that AMD specific flags can be
   * picked up.
   */
  uint32_t _featurev[kMaxCPUFeatureReg];
  /* Define the set of extended control register (XCR) values.
   */
  uint64_t _xcrv[kMaxCPUXCRReg];
  /* Define a string to hold and cache the CPUID. In such cases, such races
   * will at worst cause the CPUID to not be recognized.
   */
  char _wlid[kCPUIDStringLength];
} NaClCPUData;

/* Collect CPU data about this CPU, and put into the given data structure.
 */
void NaClCPUDataGet(NaClCPUData* data);

/* Clear cpu check state fields (i.e. set all fields to false). */
void NaClClearCPUFeatures(NaClCPUFeaturesX86 *features);

/* Set a feature. */
void NaClSetCPUFeature(NaClCPUFeaturesX86 *features, NaClCPUFeatureID id,
                       int state);

/* Query whether a feature is supported. */
static INLINE int NaClGetCPUFeature(const NaClCPUFeaturesX86 *features,
                                    NaClCPUFeatureID id) {
  return features->data[id];
}

/* Get the features for the CPU this code is running on. */
void NaClGetCurrentCPUFeatures(NaClCPUFeaturesX86 *cpu_features);

EXTERN_C_END

#endif /* NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_X86_NACL_CPUID_H_ */
