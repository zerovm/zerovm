/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime logging code.
 */
#include "include/portability_io.h"
#include "include/portability_string.h"

#include <limits.h>
#include "src/platform/nacl_log.h"

#define THREAD_SAFE_DETAIL_CHECK  0
/*
 * If set, check detail_level without grabbing a mutex.  This makes
 * logging much cheaper, but implies that the verbosity level should
 * only be changed prior to going multithreaded.
 */

#include "src/gio/gio.h"
#include "src/platform/nacl_exit.h"
#include "src/platform/nacl_timestamp.h"

/*
 * Three implementation strategies for module-specific logging:
 *
 * If Thread Local Storage is available, NaClLogSetModule sets a TLS
 * variable to the current module name and NaClLogDoLogAndUnsetModule
 * will use that variable to determine whether to log or not, without
 * taking a lock.
 *
 * If Thread-Specific Data is available (pthread.h), then
 * NaClLogModuleInit allocates a pthread_[sg]etspecific key, and
 * NaClLogSetModule / NaClLogDoLogAndUnsetModule uses the TSD variable
 * in much the same way that a TLS variable would be used.
 *
 * If neither TLS nor TSD is available, then a global variable is used
 * to hold the module name, and a NaClMutex lock is used to prevent
 * another thread from changing it until the detail level vs verbosity
 * level check has fired.
 */
#if NACL_PLATFORM_HAS_TLS
THREAD char const       *gTls_ModuleName = NULL;
#elif NACL_PLATFORM_HAS_TSD
#  include <pthread.h>
pthread_key_t           gModuleNameKey;
#else
static char const       *nacl_log_module_name = NULL;
#endif

#define NACL_VERBOSITY_UNSET INT_MAX

static int              verbosity = NACL_VERBOSITY_UNSET;
static struct Gio       *log_stream = NULL;
static struct GioFile   log_file_stream;
//static int              timestamp_enabled = 1; /* d'b(LOG): stamp disabled */

/* global, but explicitly not exposed in non-test header file */
void (*gNaClLogAbortBehavior)(void) = NaClAbort;

/*
 * For now, we use a simple linked list.  New entries are pushed to
 * the front; search starts at front.  So last entry for a particular
 * module wins, and we don't bother to eliminate duplicates.  The
 * expected number of modules is small, so we don't do anything
 * fancier.  TODO(bsy): measure performance loss and consider
 * alternatives.
 */

struct NaClLogModuleVerbosity {
  struct NaClLogModuleVerbosity *next;
  char const                    *module_name;  /* strdup'd */
  int                           verbosity;
};

static struct NaClLogModuleVerbosity *gNaClLogModuleVerbosity = NULL;

static FILE *NaClLogFileIoBufferFromFile(char const *log_file) {
  int   log_desc;
  FILE  *log_iob;

  log_desc = open(log_file, O_WRONLY | O_APPEND | O_CREAT, 0777);
  if (-1 == log_desc) {
    perror("NaClLogSetFile");
    fprintf(stderr, "Could not create log file\n");
    NaClAbort();
  }

  log_iob = FDOPEN(log_desc, "a");
  if (NULL == log_iob) {
    perror("NaClLogSetFile");
    fprintf(stderr, "Could not fdopen log stream\n");
    NaClAbort();
  }
  return log_iob;
}

/*
 * Setting the log stream buffering to fully buffered, so that the
 * write of the tag string will be less likely to be separated
 * from the write of the actual log message.
 */
static FILE *NaClLogDupFileIo(FILE *orig) {
  int  d;
  FILE *copy;

  /*
   * On windows (at least on a win7 machine which i tested on),
   * fileno(stderr) is -2.  I/O to the stderr stream appears to
   * succeed -- though who knows, maybe fclose(stderr) would actually
   * report an error? -- but DUP of -2 fails.  We don't try to detect
   * -2 (or other windows magic values) as a special case here, since
   * in the future other FILE* might be used here.  Instead, we just
   * check for DUP failure and trundle on as best as we could.
   */
  if (-1 == (d = DUP(fileno(orig)))) {
    copy = orig;
    /* this means that setvbuf later will affect the shared stream */
  } else if (NULL == (copy = FDOPEN(d, "a"))) {
    copy = orig;
    /* ditto */
  }
  (void) setvbuf(copy, (char *) NULL, _IOFBF, 1024);
  return copy;
}

static struct Gio *NaClLogGioFromFileIoBuffer(FILE *log_iob) {
  struct GioFile *log_gio;

  log_gio = malloc(sizeof *log_gio);
  if (NULL == log_gio) {
    perror("NaClLogSetFile");
    fprintf(stderr, "No memory for log buffers\n");
    NaClAbort();
  }
  if (!GioFileRefCtor(log_gio, log_iob)) {
    fprintf(stderr, "NaClLog module internal error: GioFileRefCtor failed\n");
    NaClAbort();
  }
  return (struct Gio *) log_gio;
}

void NaClLogSetFile(char const *log_file) {
  NaClLogSetGio(NaClLogGioFromFileIoBuffer(
      NaClLogFileIoBufferFromFile(log_file)));
}

int NaClLogDefaultLogVerbosity() {
  return 0;
}

struct Gio *NaClLogDefaultLogGio() {
  return NaClLogGioFromFileIoBuffer( NaClLogDupFileIo(stderr) );
}

void NaClLogParseAndSetModuleVerbosityMap(char const *module_verbosity_map) {
  char        entry_buf[256];
  size_t      entry_len;
  char const  *sep;
  char const  *next;
  char        *assign;
  int         seen_global = 0;
  char        *module_name;
  int         module_verbosity;

  if (NULL == module_verbosity_map) {
    return;
  }

  while (*module_verbosity_map != '\0') {
    sep = strpbrk(module_verbosity_map, ",:");
    if (NULL == sep) {
      sep = module_verbosity_map + strlen(module_verbosity_map);
      next = sep;
    } else {
      next = sep + 1;
    }
    /* post: sep points to comma or termination NUL */
    entry_len = sep - module_verbosity_map;
    if (entry_len > sizeof entry_buf - 1) {
      NaClLog(LOG_ERROR,
              "NaClLog: entry too long in module verbosity map \"%.*s\".\n",
              (int) entry_len,
              module_verbosity_map);
      entry_len = sizeof entry_buf - 1;
    }
    strncpy(entry_buf, module_verbosity_map, entry_len);
    entry_buf[entry_len] = '\0';
    assign = strchr(entry_buf, '=');
    if (NULL == assign && !seen_global) {
      verbosity = strtol(entry_buf, (char **) 0, 0);
      seen_global = 1;
    } else {
      *assign = '\0';

      module_verbosity = strtol(assign+1, (char **) 0, 0);

      while (entry_buf < assign && (' ' == assign[-1] || '\t' == assign[-1])) {
        *--assign = '\0';
      }
      if (entry_buf == assign) {
        NaClLog(LOG_FATAL,
                "NaClLog: Bad module name in \"%s\".\n",
                module_verbosity_map);
      }

      for (module_name = entry_buf;
           ' ' == *module_name || '\t' == *module_name;
           ++module_name) {
        ;
      }
      NaClLogSetModuleVerbosity(module_name, module_verbosity);
    }
    module_verbosity_map = next;
  }
}

void NaClLogModuleInitExtended2(int         default_verbosity,
                                char const  *module_verbosity_map,
                                struct Gio  *log_gio) {

#if !THREAD_SAFE_DETAIL_CHECK && !NACL_PLATFORM_HAS_TLS && NACL_PLATFORM_HAS_TSD
  int errnum;

  if (0 != (errnum = pthread_key_create(&gModuleNameKey, NULL))) {
    fprintf(stderr, "NaClLogModuleInitExtended2: pthread_key_create failed\n");
    abort();
  }
#endif
  NaClLogSetVerbosity(default_verbosity);
  NaClLogParseAndSetModuleVerbosityMap(module_verbosity_map);
  NaClLogSetGio(log_gio);
}

void NaClLogModuleInitExtended(int initial_verbosity, struct Gio *log_gio)
{
  NaClLogModuleInitExtended2(initial_verbosity, NULL, log_gio);
}

void NaClLogModuleInit(void) {
  NaClLogModuleInitExtended(NaClLogDefaultLogVerbosity(),
                            NaClLogDefaultLogGio());
}

void NaClLogModuleFini(void) {
  struct NaClLogModuleVerbosity *entry;
  struct NaClLogModuleVerbosity *next;

  entry = gNaClLogModuleVerbosity;
  while (entry != NULL) {
    next = entry->next;
    free(entry);
    entry = next;
  }
  gNaClLogModuleVerbosity = NULL;
}

void  NaClLogSetVerbosity(int verb) {
  verbosity = verb;
}

void  NaClLogIncrVerbosity(void) {
  if (NACL_VERBOSITY_UNSET == verbosity) {
    verbosity = 0;
  }
  ++verbosity;
}

int NaClLogGetVerbosity(void) {
  int v;

  if (NACL_VERBOSITY_UNSET == verbosity) {
    verbosity = 0;
  }
  v = verbosity;

  return v;
}

void NaClLogSetGio(struct Gio *stream) {
  if(NULL != log_stream)
  {
    (void) (*log_stream->vtbl->Flush)(log_stream);
  }
  log_stream = stream;
}

struct Gio  *NaClLogGetGio(void) {
  if (NULL == log_stream) {
    (void) GioFileRefCtor(&log_file_stream, NaClLogDupFileIo(stderr));
    log_stream = (struct Gio *) &log_file_stream;
  }
  return log_stream;
}

static void NaClLogOutputTag(struct Gio *s) {
  /* d'b(LOG) */
//  char timestamp[128];
//  int  pid;
//
//  if (timestamp_enabled) {
//    pid = GETPID();
//    gprintf(s, "[%d,%u:%s] ",
//            pid,
//            NaClTimeStampString(timestamp, sizeof timestamp));
//  }
}

/*
 * Output a printf-style formatted message if the log verbosity level
 * is set higher than the log output's detail level.  Note that since
 * this is not a macro, log message arguments that have side effects
 * will have their side effects regardless of whether the
 * corresponding log message is printed or not.  This is good from a
 * consistency point of view, but it means that should a logging
 * argument be expensive to compute, the log statement needs to be
 * surrounded by something like
 *
 *  if (detail_level <= NaClLogGetVerbosity()) {
 *    NaClLog(detail_level, "format string", expensive_arg(), ...);
 *  }
 *
 * The log message, if written, is prepended by a microsecond
 * resolution timestamp on linux and a millisecond resolution
 * timestamp on windows.  This means that if the NaCl app can read its
 * own logs, it can distinguish which host OS it is running on.
 */
void NaClLogDoLogV(int         detail_level,
                      char const  *fmt,
                      va_list     ap) {
  struct Gio  *s;

  s = NaClLogGetGio();

  NaClLogOutputTag(s);
  (void) gvprintf(s, fmt, ap);
  (void) (*s->vtbl->Flush)(s);

  if (LOG_FATAL == detail_level) {
    NaClAbort();
  }
}

void NaClLogV(int         detail_level,
              char const  *fmt,
              va_list     ap) {
#if !THREAD_SAFE_DETAIL_CHECK
  if (detail_level > verbosity) {
    return;
  }
#endif
  if (NACL_VERBOSITY_UNSET == verbosity) {
    verbosity = NaClLogDefaultLogVerbosity();
  }

  if (detail_level <= verbosity) {
    NaClLogDoLogV(detail_level, fmt, ap);
  }
}

void NaClLogSetModuleVerbosity(char const *module_name,
                               int        verbosity) {
  struct NaClLogModuleVerbosity *entry;

  entry = (struct NaClLogModuleVerbosity *) malloc(sizeof *entry);
  if (NULL == entry) {
    NaClLog(LOG_FATAL,
               ("NaClLogSetModuleVerbosity: Out of memory while setting"
                " module record for module: %s, verbosity: %d\n"),
               module_name, verbosity);
  }
  entry->module_name = STRDUP(module_name);
  if (NULL == entry->module_name) {
    NaClLog(LOG_FATAL,
               ("NaClLogSetModuleVerbosity: Out of memory while duplicating"
                " module name: %s, verbosity: %d\n"),
               module_name, verbosity);
  }
  entry->verbosity = verbosity;
  entry->next = gNaClLogModuleVerbosity;
  gNaClLogModuleVerbosity = entry;
}

int NaClLogGetModuleVerbosity(char const *module_name) {
  struct NaClLogModuleVerbosity *p;

  if (NULL != module_name) {
    for (p = gNaClLogModuleVerbosity; NULL != p; p = p->next) {
      if (!strcmp(p->module_name, module_name)) {
        return p->verbosity;
      }
    }
  }
  return verbosity;
}

#if NACL_PLATFORM_HAS_TLS
int NaClLogSetModule(char const *module_name) {
  gTls_ModuleName = module_name;
  return 0;
}

static void NaClLogDoLogAndUnsetModuleV(int        detail_level,
                                        const char *fmt,
                                        va_list    ap) {
  int module_verbosity;

  module_verbosity = NaClLogGetModuleVerbosity(gTls_ModuleName);
  if (detail_level <= module_verbosity) {
    NaClLogDoLogV(detail_level, fmt, ap);
  }
  gTls_ModuleName = (char const *) NULL;
}

#elif NACL_PLATFORM_HAS_TSD
int NaClLogSetModule(char const *module_name) {
  (void) pthread_setspecific(gModuleNameKey, (void const *) module_name);
  return 0;
}

static void NaClLogDoLogAndUnsetModuleV(int        detail_level,
                                        const char *fmt,
                                        va_list    ap) {
  char const  *module_name = (char const *) pthread_getspecific(gModuleNameKey);
  int         module_verbosity;

  module_verbosity = NaClLogGetModuleVerbosity(module_name);
  if (detail_level <= module_verbosity) {
    NaClLogLock();
    NaClLogDoLogV(detail_level, fmt, ap);
    NaClLogUnlock();
  }
  (void) pthread_setspecific(gModuleNameKey, (void const *) NULL);
}

#else
/* !NACL_PLATFORM_HAS_TLS && !NACL_PLATFORM_HAS_TSD */

static void NaClLogDoLogAndUnsetModuleV(int         detail_level,
                                        char const  *fmt,
                                        va_list     ap) {
  int module_verbosity;

  module_verbosity = NaClLogGetModuleVerbosity(nacl_log_module_name);
  if (detail_level <= module_verbosity) {
    NaClLogDoLogV(detail_level, fmt, ap);
  }
  nacl_log_module_name = NULL;
  NaClLogUnlock();
}
#endif

void NaClLogDoLogAndUnsetModule(int        detail_level,
                                char const *fmt,
                                ...) {
  va_list ap;

  va_start(ap, fmt);
  NaClLogDoLogAndUnsetModuleV(detail_level, fmt, ap);
  va_end(ap);
}

/* d'b(LOG): accessor for "current_detail_level" {{ */
static int current_detail_level;
int CurrentDetailLevel()
{
  return current_detail_level;
}

void NaClLog(int detail_level, char const *fmt, ...)
{
  va_list ap;

#if !THREAD_SAFE_DETAIL_CHECK
  if (detail_level > verbosity) return;
#endif

  current_detail_level = detail_level;

  va_start(ap, fmt);
  NaClLogV(detail_level, fmt, ap);
  va_end(ap);
}

///* d'b: conditional log */
//void LogIf(int cond, int detail_level, char const *fmt, ...)
//{
//  va_list ap;
//
//  if(cond)
//  {
//    current_detail_level = detail_level;
//
//    va_start(ap, fmt);
//    NaClLogV(detail_level, fmt, ap);
//    va_end(ap);
//  }
//}

/* d'b: log with "LOG_ERROR" if condition is true  */
void LogIf(int cond, char const *fmt, ...)
{
  va_list ap;

  if(cond)
  {
    current_detail_level = LOG_ERROR;

    va_start(ap, fmt);
    NaClLogV(current_detail_level, fmt, ap);
    va_end(ap);
  }
}

/* d'b: log with "LOG_FATAL" (aborts zerovm) if condition is true  */
void FailIf(int cond, char const *fmt, ...)
{
  va_list ap;

  if(cond)
  {
    current_detail_level = LOG_FATAL;

    va_start(ap, fmt);
    NaClLogV(current_detail_level, fmt, ap);
    va_end(ap);
  }
}

/* todo(d'b): make RetIf() if possible. return current function if condition is true */

/* }} */









