/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl logging module.
 *
 * This module is used in many parts of Native Client to generate
 * logging output.
 *
 * If you want logging output to show up, generally you would
 *
 * (1) include the lines
 *
 * #define NACL_LOG_MODULE_NAME "my_module"
 * #include "src/platform/nacl_log.h"
 *
 * in your files from which you want to generate logging output.  If
 * the NACL_LOG_MODULE_NAME preprocessor symbol is not defined, then a
 * global verbosity level is used for the logging statements' outputs.
 * NB: the definition of NACL_LOG_MODULE_NAME *must* precede the
 * include.
 *
 * Then
 *
 * (2) include actual logging output lines such as
 *
 * NaClLog(3, "Hello %s\n", "world");
 *
 * The 3 is a "detail level" for this particular log message, the
 * "Hello %s\n" is the familiar printf-style format string, and all
 * subsequent arguments are the corresponding arguments.  Type
 * checking for these arguments are done when gcc-compatible compilers
 * are used.
 *
 * When running the program (assuming the main function invoked the
 * module initializer(s) appropriately [see NaClLogModuleInit*
 * below]), you can set the verbosity level for each module, as well
 * as the global verbosity level (source files for which
 * NACL_LOG_MODULE_NAME was not defined) by running the program with
 * the NACLVERBOSITY environment variable, viz:
 *
 *   NACLVERBOSITY=3:elf_util=4:my_module=5
 *
 * would set the default verbosity level to be 3, the verbosity level
 * for the elf_util module to be 4, and the verbosity level for the
 * my_module module to be 5.
 *
 * NB: because of craziness in the test infrastructure, we accept
 * either ',' or ':' as separators.  (Setting osenv in the SCons
 * python code with NACLVERBOSITY=3,elf_util=4 would set two
 * environment variables "NACLVERBOSITY" and "elf_util", despite the
 * fact that osenv is a list of name=value strings.)
 *
 * In this case, when the desired verbosity level for my_module is 5,
 * all logging output at detail level 5 and below would be printed to
 * the log output, and logging statements with a higher detail level
 * would be suppressed.
 *
 * Pre-defined logging detail levels of LOG_INFO, LOG_WARNING,
 * LOG_ERROR, and LOG_FATAL may also be used.  When LOG_FATAL is used,
 * the application aborts after the log message is generated.
 * Messages at these levels cannot be suppressed.
 *
 *
 * The default logging output is standard error.  NB: on Windows, both
 * stdout and stderr are discarded for Windowed applications.
 *
 * All logging output may be redirected to a file using the NACLLOG
 * environment variable, the value of which should be the path to a
 * file.  NOTE: when running in Chrome, the outer sandbox is by
 * default enabled, so the log module will be unable to open the log
 * file.  In order to enable this for testing, use the --no-sandbox
 * flag to Chrome.  (This is not recommended for normal use, since it
 * eliminates a layer of defense.)
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_LOG_H__
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_LOG_H__

#include <stdarg.h>

#ifdef __native_client__
# define ATTRIBUTE_FORMAT_PRINTF(m, n) __attribute__((format(printf, m, n)))
# define NACL_PLATFORM_HAS_TLS  1
# define NACL_PLATFORM_HAS_TSD  1

/* TODO(sehr): move these defs to a common header */
#ifndef DOXYGEN_SHOULD_SKIP_THIS
/*
 * Avoid emacs' penchant for auto-indenting extern "C" blocks.
 */
#  ifdef __cplusplus
#    define EXTERN_C_BEGIN extern "C" {
#    define EXTERN_C_END   }
#  else
#    define EXTERN_C_BEGIN
#    define EXTERN_C_END
#  endif  /* __cplusplus */
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#else
# include "include/portability.h"
# if NACL_LINUX
#  define NACL_PLATFORM_HAS_TLS 1
#  define NACL_PLATFORM_HAS_TSD 1
# else
#  error "what platform are we on?"
# endif
#endif

struct Gio;

EXTERN_C_BEGIN

/*
 * "Internal" functions.  NaClLogSetModule and
 * NaClLogDoLogAndUnsetModule should only be used by the syntactic
 * macro below.
 *
 * NaClLogSetModule always return 0.  This means in the macro, its
 * continuation is the else clause.  NaClLogDoLogAndUnsetModule is a
 * "naked" identifier in the macro definition, and so will take on the
 * argument list from the apparent argument list of NaClLog.
 */
int NaClLogSetModule(char const *module_name);

/*
 * TODO: per-module logging, adding a module-name parameter, probably
 * an atom that is statically allocated.
 */
void NaClLogModuleInit(void);

void NaClLogModuleInitExtended(int        initial_verbosity,
                               struct Gio *log_gio);

void NaClLogModuleInitExtended2(int         default_verbosity,
                                char const  *module_verbosity_map,
                                struct Gio  *log_gio);

/*
 * The module verbosity map is a character string, typically from a
 * command-line argument or from the NACLVERBOSITY environment
 * variable, which has the following form:
 *
 * ([0-9]+,)?([A-Za-z_][-A-Za-z_0-9]*=[0-9]+,)*([A-Za-z_][-A-Za-z_0-9]*=[0-9]+)
 *
 * The initial digit sequence is the default verbosity level when the
 * module name isn't defined, and the rest of the comma-separated
 * sequences are name=verbosity pairs.
 *
 * The parser isn't necessarily strict.  (It permits whitespace around
 * the module name.)
 *
 * EXAMPLE USAGE:
 *
 *   module_verbosity_map="3,elf_util=4,plugin=6"
 *
 * would set the default verbosity level to 3, the verbosity levels
 * for the "elf_util" module to be 4, and for the "plugin" module to
 * be 6.
 *
 * The NaClLogModuleInit* functions take care of invoking this
 * function using the value of NACLLOG, so the normal log user should
 * not have to use this.
 */
void NaClLogParseAndSetModuleVerbosityMap(char const *module_verbosity_map);

/*
 * Convenience functions, in case only one needs to be overridden.
 * Also useful for setting a new default, e.g., invoking
 * NaClLogPreInitSetVerbosity with the maximum of the verbosity level
 * supplied from higher level code such as chrome's command line
 * flags, and the default value from the environment as returned by
 * NaClLogDefaultLogVerbosity().
 */
int NaClLogDefaultLogVerbosity();
struct Gio *NaClLogDefaultLogGio();

/*
 * Sets the log file to the named file.  Aborts program if the open
 * fails.  A GioFile object is associated with the file.
 *
 * The GioFile object is dynamically allocated, so caller is
 * responsible for obtaining it via NaClLogGetGio and freeing it as
 * appropriate, since otherwise a memory leak will occur.  This
 * includes closing the wrapped FILE *, if appropriate (e.g., not the
 * default of stderr).
 */
void NaClLogSetFile(char const *log_file);

void NaClLogModuleFini(void);

/**@param verb Verbosity max allowed value is INT_MAX-1*/
void NaClLogSetVerbosity(int verb);

int NaClLogGetVerbosity(void);

void NaClLogIncrVerbosity(void);

void NaClLogSetGio(struct Gio *out_stream);

struct Gio *NaClLogGetGio(void);

void NaClLogSetModuleVerbosity(char const *module_name,
                               int        verbosity);

int NaClLogGetModuleVerbosity(char const *module_name);

/*
 * Users of NaClLogV should add ATTRIBUTE_FORMAT_PRINTF(m,n) to their
 * function prototype, where m is the argument position of the format
 * string and n is the position of the first argument to be consumed
 * by the format specifier.
 */
void NaClLogV(int         detail_level,
              char const  *fmt,
              va_list     ap);

/* accessor for "current_detail_level" */
int CurrentDetailLevel();

void NaClLog(int         detail_level,
             char const  *fmt,
             ...) ATTRIBUTE_FORMAT_PRINTF(2, 3);

/*
 * "Internal" functions.  NaClLogSetModule and
 * NaClLogDoLogAndUnsetModule should only be used by the syntactic
 * macro below.
 *
 * NaClLogSetModule always return 0.  This means in the macro, its
 * continuation is the else clause.  NaClLogDoLogAndUnsetModule is a
 * "naked" identifier in the macro definition, and so will take on the
 * argument list from the apparent argument list of NaClLog.
 */
void NaClLogDoLogAndUnsetModule(int        detail_level,
                                char const *fmt,
                                ...) ATTRIBUTE_FORMAT_PRINTF(2, 3);

#ifdef NACL_LOG_MODULE_NAME
# define NaClLog                              \
  if (NaClLogSetModule(NACL_LOG_MODULE_NAME)) \
    ;                                         \
  else                                        \
    NaClLogDoLogAndUnsetModule

/*
 * User code has lines of the form
 *
 * NaClLog(detail_level, format_string, ...);
 *
 * We need to be usable without variadic macros.  So, when
 * NACL_LOG_MODULE_NAME is not defined, the NaClLog statement would
 * just invoke the NaClLog function.  When NACL_LOG_MODULE_NAME *is*
 * defined, however, it expands to:
 *
 * if (NaClLogSetModule(NACL_LOG_MODULE_NAME))
 *   ;
 * else
 *   NaClLogDoLogAndUnsetModule(detail_level, format_string, ...);
 *
 * Note that this is a syntactic macro, so that if the original code had
 *
 * if (foo)
 *   if (bar)
 *     NaClLog(LOG_WARNING, "EEeeep!\n");
 *   else
 *     printf("!bar\n");
 *
 * the macro expansion for NaClLog would not cause the "else" clauses
 * to mis-bind.
 *
 * Also note that the compiler may generate a warning/suggestion to
 * use braces.  This doesn't occur in the NaCl code base (esp w/
 * -Werror), but may have an impact on untrusted code that use this
 * module.
 */
#endif

#define GETPID getpid /* from removed header */

#define LOG_INFO    (-1)
#define LOG_WARNING (-2)
#define LOG_ERROR   (-3)
#define LOG_FATAL   (-4)

/*
 * Low-level logging code that requires manual lock manipulation.
 * Should be used carefully!
 */
void NaClLogLock(void);
void NaClLogUnlock(void);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_LOG_H__ */
