/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <string.h>

#include "native_client/src/trusted/fault_injection/fault_injection.h"

#include "native_client/src/include/portability.h"
#include "native_client/src/include/portability_string.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

#if NACL_LINUX
# define NACL_HAS_TLS     1  /* 1 normally, 0 to test OSX case on linux */
# define NACL_HAS_TSD     1
# define NACL_HAS_STRNDUP 1
#elif NACL_OSX
# define NACL_HAS_TLS     0
# define NACL_HAS_TSD     1
# define NACL_HAS_STRNDUP 0
#elif NACL_WINDOWS
# define NACL_HAS_TLS     1
# define NACL_HAS_TSD     0
# define NACL_HAS_STRNDUP 0
#endif

#define NACL_FAULT_INJECT_ASSUME_HINT_CORRECT 1

#if NACL_HAS_TSD
# include <pthread.h>
#endif

#if !NACL_HAS_STRNDUP
static char *strndup(char const *s, size_t n) {
  char *d = (char *) malloc(n + 1);
  CHECK(NULL != d);
  strncpy(d, s, n);
  d[n] = '\0';
  return d;
}
#endif

struct NaClFaultExpr {
  int       pass;  /* bool */
  uintptr_t fault_value;  /* only if !pass */
  uintptr_t count;  /* size_t, but uintptr_t to re-use digit parser */
};

struct NaClFaultInjectInfo {
  char const                  *call_site_name;
  int                         thread_specific_p;
  /*
   * bool: true if we should use thread specific counter; global
   * counter otherwise.
   */
  struct NaClFaultExpr        *expr;
  size_t                      num_expr;
  size_t                      size_expr;
};

/* array of NaClFaultInjectInfo */
static struct NaClFaultInjectInfo *gNaClFaultInjectInfo = NULL;
/* number in use */
static size_t                     gNaClNumFaultInjectInfo = 0;
/* number allocated */
static size_t                     gNaClSizeFaultInjectInfo = 0;

/*
 * Increment count_in_expr until we reach count in NaClFaultExpr, then
 * "carry" by resetting count_in_expr to 0 and incrementing expr_ix to
 * move to the next NaClFaultExpr.  When expr_ix reaches num_expr, we
 * have exhausted the fault_control_expression and should just pass
 * all calls through to the real function.  This makes call-site
 * processing constant time, once the call-site's entry is found.
 */
struct NaClFaultInjectCallSiteCount {
  size_t            expr_ix;
  size_t            count_in_expr;
};

/*
 * Array of call site counters indexed by position of call site name in
 * the NaClFaultInjectInfo list, for global fault_control_expressions.
 *
 * This array contains call sites that are explicitly mentioned by the
 * NACL_FAULT_INJECTION environment variable, not all call-site names
 * used in the code base.
 */
struct NaClFaultInjectCallSiteCount *gNaClFaultInjectCallSites = 0;
struct NaClMutex                    *gNaClFaultInjectMu = 0;
/* global counters mu */


#if NACL_HAS_TLS
static THREAD
struct NaClFaultInjectCallSiteCount *gTls_FaultInjectionCount = NULL;
static THREAD
uintptr_t                           gTls_FaultInjectValue = 0;
#elif NACL_HAS_TSD
pthread_key_t                       gTsd_FaultInjectCountKey;
pthread_key_t                       gTsd_FaultInjectValueKey;
#else
# error "Cannot implement thread-specific counters for fault injection"
#endif

static void NaClFaultInjectGrowIfNeeded(void) {
  size_t                      new_size;
  struct NaClFaultInjectInfo  *info;
  if (gNaClNumFaultInjectInfo < gNaClSizeFaultInjectInfo) {
    return;
  }
  new_size = 2 * gNaClSizeFaultInjectInfo;
  if (0 == new_size) {
    new_size = 4;
  }
  if (new_size > (~(size_t) 0) / sizeof *info) {
    NaClLog(LOG_FATAL, "Too many fault injection records\n");
  }
  info = (struct NaClFaultInjectInfo *) realloc(gNaClFaultInjectInfo,
                                                new_size * sizeof *info);
  CHECK(NULL != info);
  gNaClFaultInjectInfo = info;
}

/*
 * Takes ownership of the contents of |entry|.
 */
static void NaClFaultInjectAddEntry(struct NaClFaultInjectInfo const *entry) {
  NaClFaultInjectGrowIfNeeded();
  gNaClFaultInjectInfo[gNaClNumFaultInjectInfo++] = *entry;
}

static void NaClFaultInjectAllocGlobalCounters(void) {
  size_t ix;

  gNaClFaultInjectCallSites = (struct NaClFaultInjectCallSiteCount *)
      malloc(gNaClNumFaultInjectInfo * sizeof *gNaClFaultInjectCallSites);
  CHECK(NULL != gNaClFaultInjectCallSites);
  gNaClFaultInjectMu = (struct NaClMutex *) malloc(
      gNaClNumFaultInjectInfo * sizeof *gNaClFaultInjectMu);
  CHECK(NULL != gNaClFaultInjectMu);
  for (ix = 0; ix < gNaClNumFaultInjectInfo; ++ix) {
    gNaClFaultInjectCallSites[ix].expr_ix = 0;
    gNaClFaultInjectCallSites[ix].count_in_expr = 0;

    NaClXMutexCtor(&gNaClFaultInjectMu[ix]);
  }
}

static void NaClFaultInjectFreeGlobalCounters(void) {
  size_t ix;

  free(gNaClFaultInjectCallSites);
  gNaClFaultInjectCallSites = NULL;
  for (ix = 0; ix < gNaClNumFaultInjectInfo; ++ix) {
    NaClMutexDtor(&gNaClFaultInjectMu[ix]);
  }
  free(gNaClFaultInjectMu);
}

#if NACL_HAS_TLS
static struct NaClFaultInjectCallSiteCount *NaClFaultInjectFindThreadCounter(
    size_t counter_ix) {
  CHECK(counter_ix < gNaClNumFaultInjectInfo);

  if (NULL == gTls_FaultInjectionCount) {
    struct NaClFaultInjectCallSiteCount *counters;
    size_t                              ix;

    counters = (struct NaClFaultInjectCallSiteCount *)
        malloc(gNaClNumFaultInjectInfo * sizeof *counters);
    CHECK(NULL != counters);

    for (ix = 0; ix < gNaClNumFaultInjectInfo; ++ix) {
      counters[ix].expr_ix = 0;
      counters[ix].count_in_expr = 0;
    }
    gTls_FaultInjectionCount = counters;
  }

  return &gTls_FaultInjectionCount[counter_ix];
}

static void NaClFaultInjectionSetValue(uintptr_t location) {
  gTls_FaultInjectValue = location;
}

static size_t NaClFaultInjectionGetValue(void) {
  return gTls_FaultInjectValue;
}

void NaClFaultInjectPreThreadExitCleanup(void) {
  free(gTls_FaultInjectionCount);
  gTls_FaultInjectionCount = NULL;
}

#elif NACL_HAS_TSD
static struct NaClFaultInjectCallSiteCount *NaClFaultInjectFindThreadCounter(
    size_t counter_ix) {
  struct NaClFaultInjectCallSiteCount *counters;

  CHECK(counter_ix < gNaClNumFaultInjectInfo);

  counters = (struct NaClFaultInjectCallSiteCount *) pthread_getspecific(
      gTsd_FaultInjectCountKey);
  if (NULL == counters) {
    size_t                              ix;

    counters = (struct NaClFaultInjectCallSiteCount *)
        malloc(gNaClNumFaultInjectInfo * sizeof *counters);
    CHECK(NULL != counters);

    for (ix = 0; ix < gNaClNumFaultInjectInfo; ++ix) {
      counters[ix].expr_ix = 0;
      counters[ix].count_in_expr = 0;
    }
    CHECK(0 == pthread_setspecific(gTsd_FaultInjectCountKey,
                                   (void *) counters));
  }
  return &counters[counter_ix];
}

static void NaClFaultInjectCallSiteCounterDtor(void *value) {
  free((void *) value);
}

static void NaClFaultInjectionSetValue(uintptr_t value) {
  CHECK(0 == pthread_setspecific(gTsd_FaultInjectValueKey, (void *) value));
}

static uintptr_t NaClFaultInjectionGetValue(void) {
  return (uintptr_t) pthread_getspecific(gTsd_FaultInjectValueKey);
}

void NaClFaultInjectPreThreadExitCleanup(void) {
  /*
   * pthread_key_create registered NaClFaultInjectCallSiteCounterDtor.
   */
  return;
}

#else
# error "Cannot implement thread-specific counters for fault injection"
#endif

/*
 * Fault Control Expression Parser functions.  Returns parsing success
 * or fail.
 */

/*
 * NaClFaultInjectionParseHelperAddFaultExpr adds a new NaClFaultExpr
 * |expr| to the NaClFaultInjectInfo object in |out_info|, growing the
 * array of NaClFaultExpr as needed.
 */
static void NaClFaultInjectionParseHelperAddFaultExpr(
    struct NaClFaultInjectInfo  *out_info,
    struct NaClFaultExpr        *expr) {
  size_t                new_count;
  struct NaClFaultExpr  *new_exprs;

  if (out_info->num_expr == out_info->size_expr) {
    new_count = 2 * out_info->size_expr;
    if (0 == new_count) {
      new_count = 4;
    }
    new_exprs = (struct NaClFaultExpr *) realloc(out_info->expr,
                                                 new_count * sizeof *new_exprs);
    CHECK(NULL != new_exprs);
    out_info->expr = new_exprs;
    out_info->size_expr = new_count;
  }
  NaClLog(6,
          "NaClFaultInject: adding %c(%"NACL_PRIdPTR
          ",%"NACL_PRIuPTR") at %"NACL_PRIdS"\n",
          expr->pass ? 'P' : 'F', expr->fault_value, expr->count,
          out_info->num_expr);
  out_info->expr[out_info->num_expr++] = *expr;
}

/*
 * NaClFaultInjectionParseNumber consumes numbers (<count> or <value>)
 * from |*digits|, advancing the in/out pointer to the character that
 * terminates the parse.  The resultant number is put into |*out|.
 *
 * This is not a strict parser, since it permits '@' to be used for
 * the <value> non-terminal.
 */
static int NaClFaultInjectionParseNumber(uintptr_t   *out,
                                         char const  **digits) {
  char const *p = *digits;
  char       *pp;

  if ('@' == *p) {
    *out = ~(uintptr_t) 0;
    *digits = p+1;
    return 1;
  }
  *out = strtoul(p, &pp, 0);
  if (pp != p) {
    *digits = pp;
    return 1;
  }
  return 0;
}

/*
 * NaClFaultInjectionParsePassOrFailSeq consumes <pass_or_fail_seq> of
 * the grammar from |fault_ctrl| until the terminating NUL character,
 * filling out |out_info| as it goes.
 */
static int NaClFaultInjectionParsePassOrFailSeq(
    struct NaClFaultInjectInfo  *out_info,
    char const                  *fault_ctrl) {
  struct NaClFaultExpr  expr;

  for (;;) {
    switch (*fault_ctrl) {
      case '\0':
        return 1;
      case 'P':
        expr.pass = 1;
        ++fault_ctrl;
        if (!NaClFaultInjectionParseNumber(&expr.count, &fault_ctrl)) {
          expr.count = 1;
        }
        expr.fault_value = 0;  /* not used during injection execution */
        NaClFaultInjectionParseHelperAddFaultExpr(out_info, &expr);
        break;
    case 'F':
      expr.pass = 0;
      ++fault_ctrl;
      if (!NaClFaultInjectionParseNumber(&expr.fault_value, &fault_ctrl)) {
        expr.fault_value = 0;
      }
      if ('/' == *fault_ctrl) {
        ++fault_ctrl;
        if (!NaClFaultInjectionParseNumber(&expr.count, &fault_ctrl)) {
          NaClLog(LOG_ERROR,
                  "NaClLogInject: bad fault count\n");
          return 0;
        }
      } else {
        expr.count = 1;
      }
      NaClFaultInjectionParseHelperAddFaultExpr(out_info, &expr);
      break;
    default:
      NaClLog(LOG_ERROR,
              "NaClFaultInjection: expected 'P' or 'F', got '%c'\n",
              *fault_ctrl);
      return 0;
    }
    if ('+' == *fault_ctrl) {
      ++fault_ctrl;
    }
  }
}

/*
 * NaClFaultInjectionParseFaultControlExpr consumes
 * <fault_control_expression> from |fault_ctrl| until the terminating
 * ASCII NUL character, filling in |out_info|.
 */
static int NaClFaultInjectionParseFaultControlExpr(
    struct NaClFaultInjectInfo  *out_info,
    char const                  *fault_ctrl) {
  NaClLog(6, "NaClFaultInject: control sequence %s\n", fault_ctrl);
  if ('T' == *fault_ctrl) {
    out_info->thread_specific_p = 1;
    ++fault_ctrl;
  } else if ('G' == *fault_ctrl) {
    out_info->thread_specific_p = 0;
    ++fault_ctrl;
  } else {
    NaClLog(LOG_ERROR,
            "NaClFaultInjection: fault control expression should indicate"
            " if the counter is thread-local or global\n");
    /*
     * Should we default to global?
     */
    return 0;
  }
  return NaClFaultInjectionParsePassOrFailSeq(out_info, fault_ctrl);
}

static int NaClFaultInjectionParseConfigEntry(
    struct NaClFaultInjectInfo  *out_info,
    char                        *entry_start) {
  char *equal = strchr(entry_start, '=');
  if (NULL == equal) {
    NaClLog(LOG_ERROR,
            "NaClFaultInject: control entry %s malformed, no equal sign\n",
            entry_start);
    return 0;
  }
  out_info->call_site_name = strndup(entry_start, equal - entry_start);
  out_info->thread_specific_p = 0;
  out_info->expr = NULL;
  out_info->num_expr = 0;
  out_info->size_expr = 0;
  CHECK(NULL != out_info->call_site_name);

  return NaClFaultInjectionParseFaultControlExpr(out_info, equal+1);
}

void NaClFaultInjectionModuleInternalInit(void) {
  char                        *config;
  char                        *cur_entry;
  char                        *sep;
  char                        *next_entry;
  struct NaClFaultInjectInfo  fi_entry;

#if !NACL_HAS_TLS && NACL_HAS_TSD
  pthread_key_create(&gTsd_FaultInjectCountKey,
                     NaClFaultInjectCallSiteCounterDtor);
  pthread_key_create(&gTsd_FaultInjectValueKey, NULL);
#endif

  config = getenv("NACL_FAULT_INJECTION");
  if (NULL == config) {
    return;
  }
  /* get a definitely-mutable version that we will free later */
  config = STRDUP(config);
  CHECK(NULL != config);
  for (cur_entry = config; '\0' != *cur_entry; cur_entry = next_entry) {
    sep = strpbrk(cur_entry, ",:");
    if (NULL == sep) {
      sep = cur_entry + strlen(cur_entry);
      next_entry = sep;
    } else {
      *sep = '\0';
      next_entry = sep + 1;
    }
    /* parse cur_entry */
    if (!NaClFaultInjectionParseConfigEntry(&fi_entry, cur_entry)) {
      NaClLog(LOG_FATAL,
              "NaClFaultInjection: syntax error in configuration; environment"
              " variable NACL_FAULT_INJECTION contains %s, which is not"
              " syntactically correct.\n",
              cur_entry);
    }
    NaClFaultInjectAddEntry(&fi_entry);
  }
  free((void *) config);
  NaClFaultInjectAllocGlobalCounters();
}

void NaClFaultInjectionModuleInternalFini(void) {
  size_t  ix;

  NaClFaultInjectFreeGlobalCounters();
  for (ix = 0; ix < gNaClNumFaultInjectInfo; ++ix) {
    free((void *) gNaClFaultInjectInfo[ix].call_site_name);  /* strndup'd */
    free((void *) gNaClFaultInjectInfo[ix].expr);
  }
  free((void *) gNaClFaultInjectInfo);
  gNaClFaultInjectInfo = NULL;
  gNaClNumFaultInjectInfo = 0;
  gNaClSizeFaultInjectInfo = 0;
}

void NaClFaultInjectionModuleInit(void) {
  static int                  initialized = 0;

  if (initialized) {
    return;
  }
  NaClFaultInjectionModuleInternalInit();
  initialized = 1;
}

int NaClFaultInjectionFaultP(char const *site_name) {
  int                                 rv;
  struct NaClFaultInjectInfo const    *entry = NULL;
  size_t                              ix;
  struct NaClFaultInjectCallSiteCount *counter;
  struct NaClFaultExpr                *expr;

  for (ix = 0; ix < gNaClNumFaultInjectInfo; ++ix) {
    if (!strcmp(site_name, gNaClFaultInjectInfo[ix].call_site_name)) {
      NaClLog(6, "NaClFaultInject: found %s\n", site_name);
      break;
    }
  }
  if (ix == gNaClNumFaultInjectInfo) {
    return 0;
  }
  entry = &gNaClFaultInjectInfo[ix];
  if (entry->thread_specific_p) {
    NaClLog(6, "NaClFaultInject: thread-specific counter\n");
    counter = NaClFaultInjectFindThreadCounter(ix);
  } else {
    NaClLog(6, "NaClFaultInject: global counter\n");
    NaClXMutexLock(&gNaClFaultInjectMu[ix]);
    counter = &gNaClFaultInjectCallSites[ix];
  }
  /*
   * check counter against entry, and if a fault should be injected,
   * set Value for NaClFaultInjectionValue and set return value to
   * true; otherwise set return value false.  bump counter.
   */
  NaClLog(6, "NaClFaultInject: counter(%"NACL_PRIxS",%"NACL_PRIxS")\n",
          counter->expr_ix, counter->count_in_expr);
  if (counter->expr_ix >= entry->num_expr) {
    rv = 0;
  } else {
    expr = &entry->expr[counter->expr_ix];
    if (expr->pass) {
      rv = 0;
    } else {
      NaClLog(6, "NaClFaultInject: should fail, value %"NACL_PRIxPTR"\n",
              expr->fault_value);
      rv = 1;
      NaClFaultInjectionSetValue(expr->fault_value);
    }
    /* bump counter, possibly carry */
    if (++counter->count_in_expr >= expr->count) {
      counter->count_in_expr = 0;
      ++counter->expr_ix;
    }
  }
  if (!entry->thread_specific_p) {
    NaClXMutexUnlock(&gNaClFaultInjectMu[ix]);
  }
  return rv;
}

uintptr_t NaClFaultInjectionValue(void) {
  return NaClFaultInjectionGetValue();
}
