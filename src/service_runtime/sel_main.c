/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 */
#include "include/portability.h"
#include "include/portability_io.h"

#if NACL_OSX
#include <crt_externs.h>
#endif

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm || NACL_SANDBOX_FIXED_AT_ZERO == 1
/* Required for our use of mallopt -- see below. */
#include <malloc.h>
#endif

#include "src/gio/gio.h"
#include "src/imc/nacl_imc_c.h"
#include "src/platform/nacl_check.h"
#include "src/platform/nacl_exit.h"
#include "src/platform/nacl_log.h"
#include "src/platform/nacl_sync.h"
#include "src/platform/nacl_sync_checked.h"

#include "src/fault_injection/fault_injection.h"
#include "src/perf_counter/nacl_perf_counter.h"
#include "src/service_runtime/env_cleanser.h"
#include "src/service_runtime/include/sys/fcntl.h"
#include "src/service_runtime/nacl_app.h"
#include "src/service_runtime/nacl_all_modules.h"
#include "src/service_runtime/nacl_config_dangerous.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_signal.h"
#include "src/service_runtime/nacl_syscall_common.h"
#include "src/service_runtime/include/sys/mman.h" /* d'b */
#include "src/service_runtime/nacl_syscall_handlers.h" /* d'b */
#include "src/manifest/manifest_parser.h" /* d'b */
#include "src/manifest/manifest_setup.h" /* d'b */
#include "src/service_runtime/nacl_app_thread.h" /* d'b */
#include "src/manifest/mount_channel.h" /* d'b */
#include "src/service_runtime/outer_sandbox.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/sel_qualify.h"

#if (NACL_WINDOWS && NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && \
     NACL_BUILD_SUBARCH == 64)
#include "src/service_runtime/win/exception_patch/ntdll_patch.h"
#endif


static void VmentryPrinter(void           *state,
                    struct NaClVmmapEntry *vmep) {
  UNREFERENCED_PARAMETER(state);
  printf("page num 0x%06x\n", (uint32_t)vmep->page_num);
  printf("num pages %d\n", (uint32_t)vmep->npages);
  printf("prot bits %x\n", vmep->prot);
  fflush(stdout);
}

static void PrintVmmap(struct NaClApp  *nap) {
  printf("In PrintVmmap\n");
  fflush(stdout);
  NaClXMutexLock(&nap->mu);
  NaClVmmapVisit(&nap->mem_map, VmentryPrinter, (void *) 0);

  NaClXMutexUnlock(&nap->mu);
}


struct redir {
  struct redir  *next;
  int           nacl_desc;
  enum {
    HOST_DESC,
    IMC_DESC
  }             tag;
  union {
    struct {
      int d;
      int mode;
    }                         host;
    NaClHandle                handle;
    struct NaClSocketAddress  addr;
  } u;
};

int ImportModeMap(char opt) {
  switch (opt) {
    case 'h':
      return O_RDWR;
    case 'r':
      return O_RDONLY;
    case 'w':
      return O_WRONLY;
  }
  fprintf(stderr, ("option %c not understood as a host descriptor"
                   " import mode\n"),
          opt);
  exit(1);
  /* NOTREACHED */
}


static void PrintUsage() {
  /* NOTE: this is broken up into multiple statements to work around
           the constant string size limit */
  fprintf(stderr,
          "Usage: sel_ldr [-h d:D] [-r d:D] [-w d:D] [-i d:D]\n"
          "               [-f nacl_file] [-l log_file] [-X d]\n"
          "               [-M manifest_file] [-acFgIsQ] [-v d]\n" /* removed odd switches: mM */
          "               [-Y d] -- [nacl_file] [args]\n"
          "\n");
  fprintf(stderr,
          " -h\n"
          " -r\n"
          " -w associate a host POSIX descriptor D with app desc d\n"
          "    that was opened in O_RDWR, O_RDONLY, and O_WRONLY modes\n"
          "    respectively\n"
          " -i associates an IMC handle D with app desc d\n"
          " -f file to load; if omitted, 1st arg after \"--\" is loaded\n"
          " -B additional ELF file to load as a blob library\n"
          " -v <level> verbosity\n"
          " -X create a bound socket and export the address via an\n"
          "    IMC message to a corresponding NaCl app descriptor\n"
          "    (use -1 to create the bound socket / address descriptor\n"
          "    pair, but that no export via IMC should occur)\n");
  fprintf(stderr,
          "\n"
          " (testing flags)\n"
          " -a allow file access! dangerous!\n"
          " -c ignore validator! dangerous! Repeating this option twice skips\n"
          "    validation completely.\n"
          " -F fuzz testing; quit after loading NaCl app\n"
          " -S enable signal handling.  Not secure on x86-64 Windows.\n"
          " -g enable gdb debug stub.  Not secure on x86-64 Windows.\n"
          " -l <file>  write log output to the given file\n"
          " -s safely stub out non-validating instructions\n"
          " -Q disable platform qualification (dangerous!)\n"
          " -E <name=value>|<name> set an environment variable\n"
          " -Y [0..2] disable/skip/enable syscalls (dangerous!)\n" /* d'b */
		      " -M <file> load settings from manifest\n" /* d'b */
          );  /* easier to add new flags/lines */
}

int main(int argc, char **argv)
{
  int                           opt;
  char                          *rest;
  struct redir                  *entry;
  struct redir                  *redir_queue;
  struct redir                  **redir_qend;


  struct NaClApp                state;
  char                          *nacl_file = NULL;
  char                          *blob_library_file = NULL;
  int                           export_addr_to = -2;

  struct NaClApp                *nap;

  struct GioFile                gout;
  NaClErrorCode                 errcode = LOAD_INTERNAL;
  struct GioMemoryFileSnapshot  blob_file;

  int                           ret_code;
  struct DynArray               env_vars;

  char                          *log_file = NULL;
  struct GioFile                *log_gio;
  int                           log_desc;
  int                           verbosity = 0;
  int                           fuzzing_quit_after_load = 0;
  int                           debug_mode_bypass_acl_checks = 0;
  int                           debug_mode_ignore_validator = 0;
  int                           stub_out_mode = 0;
  int                           skip_qualification = 0;
  int                           enable_debug_stub = 0;
  int                           handle_signals = 0;

  /* d'b added variables */
  struct GioMemoryFileSnapshot  main_file; /* moved up from removed if() */
  int														i;
  char                          *ma_name = NULL;
  char                          manifest[MAX_MANIFEST_LEN];
  int 													nexe_argc = 1;
  char 													**nexe_argv = NULL;


  /*
   * 0 - restrict all syscalls except vital, restricted syscalls will abort the program
   * 1 - restrict all syscalls except vital, restricted syscalls will be silently ignored
   * 2 - enable all syscalls
   * TODO(d'b)
   * how to make bitmask for all syscalls and pass it through command line?
   * for 55 syscalls with 3 states for each it will be 28 x-s (xxxxxxxxxxxxxxxxxxxxxxxxxxxx)
   * where x is a hexadecimal number, so "-Y 080000000000000000000000200f" would be too much
   * for a one command line option.
   */
  char                          *syscalls_behavior = NULL;
  /* d'b end */

  struct NaClPerfCounter time_all_main;
  const char **envp;
  struct NaClEnvCleanser env_cleanser;
  const char *sandbox_fd_string;
  /* Mac dynamic libraries cannot access the environ variable directly. */
  /* Overzealous code style check is overzealous. */
  /* @IGNORE_LINES_FOR_CODE_HYGIENE[1] */
  extern char **environ;
  envp = (const char**)environ;
  /*
   * Set malloc not to use mmap even for large allocations.  This is currently
   * necessary when we must use a specific area of RAM for the sandbox.
   *
   * During startup, before the sandbox is set up, the sel_ldr allocates a chunk
   * of memory to store the untrusted code.  Normally such an allocation would
   * go into the sel_ldr's heap area, but the allocation is typically large --
   * at least hundreds of KiB.  The default malloc configuration on Linux (at
   * least) switches to mmap for such allocations, and mmap will select
   * essentially any unoccupied section of the address space.  The result: the
   * nexe is allocated in the region we use for the sandbox, we protect the
   * address space, and then the memcpy into the sandbox (of course) fails.
   *
   * This is at best a temporary fix.  The proper fix is to reserve the
   * sandbox region early enough that this isn't a problem.  Possible methods
   * are discussed in this bug:
   *   http://code.google.com/p/nativeclient/issues/detail?id=232
   */
    ret_code = 1;
    redir_queue = NULL;
    redir_qend = &redir_queue;
    /* d'b: cannot be moved. initialization failed
   * when i tried to put it below command line parser switch
   */
    NaClAllModulesInit();
    verbosity = NaClLogGetVerbosity();
    NaClPerfCounterCtor(&time_all_main, "SelMain");
    fflush((FILE *) NULL);
    if (!GioFileRefCtor(&gout, stdout)) {
    fprintf(stderr, "Could not create general standard output channel\n");
    exit(1);
  }
    if (!DynArrayCtor(&env_vars, 0)) {
    NaClLog(LOG_FATAL, "Failed to allocate env var array\n");
  }
    /*
   * On platforms with glibc getopt, require POSIXLY_CORRECT behavior,
   * viz, no reordering of the arglist -- stop argument processing as
   * soon as an unrecognized argument is encountered, so that, for
   * example, in the invocation
   *
   *   sel_ldr foo.nexe -vvv
   *
   * the -vvv flags are made available to the nexe, rather than being
   * consumed by getopt.  This makes the behavior of the Linux build
   * of sel_ldr consistent with the Windows and OSX builds.
   */
    while ((opt = getopt(argc, argv,
#if NACL_LINUX
                       "+"
#endif
                       "aB:cE:f:Fgh:i:Il:Qr:sSv:w:X:Y:M:")) != -1) {/* d'b */
    switch (opt) {
    	/* d'b: enable syscalls */
      case 'Y':
      	syscalls_behavior = optarg;
      	break;
      case 'M':
      	ma_name = optarg;
      	break;
    	/* d'b end */

      case 'c':
        ++debug_mode_ignore_validator;
        break;
      case 'a':
        fprintf(stderr, "DEBUG MODE ENABLED (bypass acl)\n");
        debug_mode_bypass_acl_checks = 1;
        break;
      case 'f':
        nacl_file = optarg;
        break;
      case 'B':
        blob_library_file = optarg;
        break;
      case 'F':
        fuzzing_quit_after_load = 1;
        break;

      case 'g':
        handle_signals = 1;
        enable_debug_stub = 1;
        break;

      case 'S':
        handle_signals = 1;
        break;

      case 'h':
      case 'r':
      case 'w':
        /* import host descriptor */
        entry = malloc(sizeof *entry);
        if (NULL == entry) {
          fprintf(stderr, "No memory for redirection queue\n");
          exit(1);
        }
        entry->next = NULL;
        entry->nacl_desc = strtol(optarg, &rest, 0);
        entry->tag = HOST_DESC;
        entry->u.host.d = strtol(rest+1, (char **) 0, 0);
        entry->u.host.mode = ImportModeMap(opt);
        *redir_qend = entry;
        redir_qend = &entry->next;
        break;
      case 'i':
        /* import IMC handle */
        entry = malloc(sizeof *entry);
        if (NULL == entry) {
          fprintf(stderr, "No memory for redirection queue\n");
          exit(1);
        }
        entry->next = NULL;
        entry->nacl_desc = strtol(optarg, &rest, 0);
        entry->tag = IMC_DESC;
        entry->u.handle = (NaClHandle) strtol(rest+1, (char **) 0, 0);
        *redir_qend = entry;
        redir_qend = &entry->next;
        break;
      case 'l':
        log_file = optarg;
        break;
      /* case 'r':  with 'h' and 'w' above */
      case 'v':
        /* d'b
         * ugly but i don't want to change much files
         */
        /* ++verbosity; */
      	i = atoi(optarg);
      	i = verbosity = i < 1 ? 0 : i;
        while(i--)
        /* d'b end */
        	NaClLogIncrVerbosity();
        break;
      /* case 'w':  with 'h' and 'r' above */
      case 'X':
        export_addr_to = strtol(optarg, (char **) 0, 0);
        break;
      case 'Q':
        fprintf(stderr, "PLATFORM QUALIFICATION DISABLED BY -Q - "
                "Native Client's sandbox will be unreliable!\n");
        skip_qualification = 1;
        break;
      case 's':
        stub_out_mode = 1;
        break;
      case 'E':
        /*
         * For simplicity, we treat the environment variables as a
         * list of strings rather than a key/value mapping.  We do not
         * try to prevent duplicate keys or require the strings to be
         * of the form "KEY=VALUE".  This is in line with how execve()
         * works in Unix.
         *
         * We expect that most callers passing "-E" will either pass
         * in a fixed list or will construct the list using a
         * high-level language, in which case de-duplicating keys
         * outside of sel_ldr is easier.  However, we could do
         * de-duplication here if it proves to be worthwhile.
         */
        if (!DynArraySet(&env_vars, env_vars.num_entries, optarg)) {
          NaClLog(LOG_FATAL, "Adding item to env_vars failed\n");
        }
        break;
      default:
        fprintf(stderr, "ERROR: unknown option: [%c]\n\n", opt);
        PrintUsage();
        exit(-1);
    }
  }

	if (debug_mode_ignore_validator == 1)
		fprintf(stderr, "DEBUG MODE ENABLED (ignore validator)\n");
	else if (debug_mode_ignore_validator > 1)
		fprintf(stderr, "DEBUG MODE ENABLED (skip validator)\n");
	if (verbosity)
	{
		int ix;
		char const *separator = "";

		fprintf(stderr, "sel_ldr argument list:\n");
		for (ix = 0; ix < argc; ++ix)
		{
			fprintf(stderr, "%s%s", separator, argv[ix]);
			separator = " ";
		}
		putc('\n', stderr);
	}

#if 0 != NACL_DANGEROUS_STUFF_ENABLED
  fprintf(stderr, "WARNING WARNING WARNING WARNING"
      " WARNING WARNING WARNING WARNING\n");
  fprintf(stderr, "WARNING\n");
  fprintf(stderr, "WARNING  Using a dangerous/debug configuration.\n");
  fprintf(stderr, "WARNING\n");
  fprintf(stderr, "WARNING WARNING WARNING WARNING"
      " WARNING WARNING WARNING WARNING\n");
#endif

	if (debug_mode_bypass_acl_checks)
	{
		NaClInsecurelyBypassAllAclChecks();
	}

	/*
	 * d'b: enable syscalls. parse manifest.
	 * check manifest resources. setup channels
	 */
	// ### todo: full main() refactoring
	// every variable should be initialized in nap object, main must be split into functions
	// it will simplify main() and will remove needlessly linked code pieces
  /* process manifest file specified in cmdline */
	if (ma_name == NULL)
	{
		state.manifest = NULL;
	}
	/* manifest is provided */
	else {
	  int32_t size;
		if (!ParseManifest(ma_name, &state))
		{
			fprintf(stderr, "Invalid manifest file \"%s\".\n", ma_name);
			exit(1);
		}

		/* initialize user policy, zerovm settings */
		SetupUserPolicy(&state);
		SetupSystemPolicy(&state);


		// ### this part must be completelly removed when command line will be replaced by manifest
	  /* check if command line switches has duplicates in manifest */
#define SPACE " \t"
#define ARGC_MAX 128*sizeof(char*)
#define SET_KEY_FROM_MANIFEST(key, var)\
	  do {\
	    char *buf = GetValueByKey(&state, key);\
	    if(var != NULL && buf != NULL)\
	    {\
	      fprintf(stderr, "command line switch tried to override a manifest key\n"\
	          "command line value = %s, manifest value = %s\n", var, buf);\
	      return 1;\
	    }\
	    if(var == NULL) var = buf;\
	  } while (0)

    /* nacl_file special case when name provided without -f */
    if(NULL == nacl_file && optind < argc) nacl_file = argv[optind++];

    SET_KEY_FROM_MANIFEST("Log", log_file);
    SET_KEY_FROM_MANIFEST("Nexe", nacl_file);
    SET_KEY_FROM_MANIFEST("Blob", blob_library_file);
    // ### this part must be completelly removed when command line will be replaced by manifest. until here

    /* construct nexe command line from manifest */
    nexe_argc = 1;
    nexe_argv = (char**) malloc(ARGC_MAX);
    nexe_argv[0] = "_"; /* not available for nexe (see minor manifest) */
    nexe_argv[nexe_argc] = strtok(GetValueByKey(&state, "CommandLine"), SPACE);

    while(nexe_argv[nexe_argc])
      nexe_argv[++nexe_argc] = strtok(NULL, SPACE);
#undef SPACE
#undef ARGC_MAX
#undef SET_KEY_FROM_MANIFEST

    // bug: nacldesc array usage before it's initialization, will be solved after removing
    // zvm command line. ###
    /* initial checking for limits from manifest */
    COND_ABORT(strcmp(state.manifest->system_setup->version, MANIFEST_VERSION),
        "wrong manifest version\n");
    size = GetFileSize(state.manifest->system_setup->nexe);
    if(size < 0)
    {
      fprintf(stderr, "%s not found\n", state.manifest->system_setup->nexe);
      return 1;
    }

    if(state.manifest->system_setup->nexe_max)
      COND_ABORT(state.manifest->system_setup->nexe_max < size, "nexe file is greater then alowed\n");

    /* ### channels initialization moved bellow because it is need initialized nacldesc dynarray */
	}

	/*
	 * change stdout/stderr to log file now, so that subsequent error
	 * messages will go there.  unfortunately, error messages that
	 * result from getopt processing -- usually out-of-memory, which
	 * shouldn't happen -- won't show up.
	 */
	if (NULL != log_file) NaClLogSetFile(log_file);

	state.enable_syscalls = 0; /* syscalls are disabled by default */
	if (syscalls_behavior == NULL || !strcmp(syscalls_behavior, "0"))
		state.enable_syscalls = 0;
	else if (!strcmp(syscalls_behavior, "1"))
		NaClSilentRestrictedSyscalls();
	else if (!strcmp(syscalls_behavior, "2"))
	{
		/* initialize syscalls */
		NaClSyscallTableInit();
		NaClLog(LOG_WARNING, "DANGER! SYSCALLS ARE ENABLED\n");
		state.enable_syscalls = 1;

		/* also enable file i/o "-a" switch */
		NaClInsecurelyBypassAllAclChecks(); // remove '-a' switch and appropriate init above
	}
	else if (strcmp(syscalls_behavior, "0"))
	{
		fprintf(stderr, "ERROR: unknown -Y option: [%s]\n", syscalls_behavior);
		PrintUsage();
		exit(-1);
	}
	/* d'b: log initialization moved above */
	/* d'b end */

  /*
   * NB: the following cast is okay since we only ever permit GioFile
   * objects to be used -- NaClLogModuleInit and NaClLogSetFile both
   * can only assign the log output to a file.  If neither were
   * called, logging goes to stderr.
   */
  log_gio = (struct GioFile*) NaClLogGetGio();
  /*
   * By default, the logging module logs to stderr, or descriptor 2.
   * If NaClLogSetFile was performed above, then log_desc will have
   * the non-default value.
   */
  log_desc = fileno(log_gio->iop);

  if (NULL == nacl_file)
  {
    fprintf(stderr, "No nacl file specified\n");
    exit(1);
  }

  /* to be passed to NaClMain, eventually... */
  argv[--optind] = (char*) "NaClMain";
  if (!NaClAppCtor(&state))
  {
    fprintf(stderr, "Error while constructing app state\n");
    goto done_file_dtor;
  }
	state.ignore_validator_result = (debug_mode_ignore_validator > 0);
	state.skip_validator = (debug_mode_ignore_validator > 1);
	state.validator_stub_out_mode = stub_out_mode;
	state.enable_debug_stub = enable_debug_stub;

  nap = &state;
	errcode = LOAD_OK;

	/*
   * in order to report load error to the browser plugin through the
   * secure command channel, we do not immediate jump to cleanup code
   * on error.  rather, we continue processing (assuming earlier
   * errors do not make it inappropriate) until the secure command
   * channel is set up, and then bail out.
   */

  /*
   * Ensure the platform qualification checks pass.
   *
   * NACL_DANGEROUS_SKIP_QUALIFICATION_TEST is used by tsan / memcheck
   * (see src/third_party/valgrind/).
   */
  if (!skip_qualification &&
      getenv("NACL_DANGEROUS_SKIP_QUALIFICATION_TEST") != NULL) {
    fprintf(stderr, "PLATFORM QUALIFICATION DISABLED BY ENVIRONMENT - "
            "Native Client's sandbox will be unreliable!\n");
    skip_qualification = 1;
  }

  /* We use the signal handler to verify a signal took place. */
	NaClSignalHandlerInit();
	if (!skip_qualification)
	{
		NaClErrorCode pq_error = NACL_FI_VAL("pq", NaClErrorCode,
				NaClRunSelQualificationTests());
		if (LOAD_OK != pq_error)
		{
			errcode = pq_error;
			nap->module_load_status = pq_error;
			fprintf(stderr, "Error while loading \"%s\": %s\n",
					NULL != nacl_file ? nacl_file : "(no file, to-be-supplied-via-RPC)",
					NaClErrorString(errcode));
		}
	}

  /* Remove the signal handler if we are not using it. */
	if (!handle_signals)
	{
		NaClSignalHandlerFini();
		/* Sanity check. */
		NaClSignalAssertNoHandlers();

		/*
		 * Patch the Windows exception dispatcher to be safe in the case
		 * of faults inside x86-64 sandboxed code.  The sandbox is not
		 * secure on 64-bit Windows without this.
		 */
#if (NACL_WINDOWS && NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && \
     NACL_BUILD_SUBARCH == 64)
		NaClPatchWindowsExceptionDispatcher();
#endif
	}

  /*
   * Open both files first because (on Mac OS X at least)
   * NaClAppLoadFile() enables an outer sandbox.
   */
  if (NULL != blob_library_file) {
    if (0 == GioMemoryFileSnapshotCtor(&blob_file, blob_library_file)) {
      perror("sel_main");
      fprintf(stderr, "Cannot open \"%s\".\n", blob_library_file);
      exit(1);
    }
    NaClPerfCounterMark(&time_all_main, "SnapshotBlob");
    NaClPerfCounterIntervalLast(&time_all_main);
  }

  if (0 == GioMemoryFileSnapshotCtor(&main_file, nacl_file))
  {
    perror("sel_main");
    fprintf(stderr, "Cannot open \"%s\".\n", nacl_file);
    exit(1);
  }
  NaClPerfCounterMark(&time_all_main, "SnapshotNaclFile");
  NaClPerfCounterIntervalLast(&time_all_main);

  if (LOAD_OK == errcode)
  {
    NaClLog(2, "Loading nacl file %s (non-RPC)\n", nacl_file);
    errcode = NaClAppLoadFile((struct Gio *) &main_file, nap);
    if (LOAD_OK != errcode)
    {
      fprintf(stderr, "Error while loading \"%s\": %s\n", nacl_file,
              NaClErrorString(errcode));
      fprintf(stderr, ("Using the wrong type of nexe (nacl-x86-32"
              " on an x86-64 or vice versa)\n"
              "or a corrupt nexe file may be"
              " responsible for this error.\n"));
    }

    NaClPerfCounterMark(&time_all_main, "AppLoadEnd");
    NaClPerfCounterIntervalLast(&time_all_main);

    NaClXMutexLock(&nap->mu);
    nap->module_load_status = errcode;
    NaClXCondVarBroadcast(&nap->cv);
    NaClXMutexUnlock(&nap->mu);
  }

  if (-1 == (*((struct Gio *) &main_file)->vtbl->Close)((struct Gio *) &main_file))
  {
    fprintf(stderr, "Error while closing \"%s\".\n", nacl_file);
  }

  (*((struct Gio *) &main_file)->vtbl->Dtor)((struct Gio *) &main_file);
  if (fuzzing_quit_after_load) exit(0);

  /* d'b: construct each mentioned in manifest channel and mount it */
  if(nap->manifest)
  {
    enum ChannelType ch;
    for(ch = InputChannel; ch < CHANNELS_COUNT; ++ch)
    {
      if(ConstructChannel(nap, ch)) continue;
      MountChannel(nap, ch);
    }
  }
  /* d'b end */

  NaClAppInitialDescriptorHookup(nap);

  /*
   * Execute additional I/O redirections.  NB: since the NaClApp
   * takes ownership of host / IMC socket descriptors, all but
   * the first run will not get access if the NaClApp closes
   * them.  Currently a normal NaClApp process exit does not
   * close descriptors, since the underlying host OS will do so
   * as part of service runtime exit.
   */
  NaClLog(4, "Processing I/O redirection/inheritance from environment\n");
  for (entry = redir_queue; NULL != entry; entry = entry->next) {
    switch (entry->tag) {
      case HOST_DESC:
        NaClAddHostDescriptor(nap, entry->u.host.d,
                              entry->u.host.mode, entry->nacl_desc);
        break;
      case IMC_DESC:
        NaClAddImcHandle(nap, entry->u.handle, entry->nacl_desc);
        break;
    }
  }

  /*
   * May have created a thread, so need to synchronize uses of nap
   * contents henceforth.
   */

  /**************************************************************************
   * TODO(bsy): This else block should be made unconditional and
   * invoked after the LoadModule RPC completes, eliminating the
   * essentially dulicated code in latter part of NaClLoadModuleRpc.
   * This cannot be done until we have full saucer separation
   * technology, since Chrome currently uses sel_main_chrome.c and
   * relies on the functionality of the duplicated code.
   *************************************************************************/
  if (LOAD_OK == errcode)
  {
    if (verbosity)
    {
      gprintf((struct Gio *) &gout, "printing NaClApp details\n");
      NaClAppPrintDetails(nap, (struct Gio *) &gout);
    }

    /*
     * Finish setting up the NaCl App.  On x86-32, this means
     * allocating segment selectors.  On x86-64 and ARM, this is
     * (currently) a no-op.
     */
    errcode = NaClAppPrepareToLaunch(nap);
    if (LOAD_OK != errcode)
    {
      nap->module_load_status = errcode;
      fprintf(stderr, "NaClAppPrepareToLaunch returned %d", errcode);
    }
    NaClPerfCounterMark(&time_all_main, "AppPrepLaunch");
    NaClPerfCounterIntervalLast(&time_all_main);
  }

  if (NULL != blob_library_file) {
    if (LOAD_OK == errcode) {
      NaClLog(2, "Loading blob file %s\n", blob_library_file);
      errcode = NaClAppLoadFileDynamically(nap, (struct Gio *) &blob_file);
      if (LOAD_OK != errcode) {
        fprintf(stderr, "Error while loading \"%s\": %s\n",
                blob_library_file,
                NaClErrorString(errcode));
      }
      NaClPerfCounterMark(&time_all_main, "BlobLoaded");
      NaClPerfCounterIntervalLast(&time_all_main);
    }

    if (-1 == (*((struct Gio *) &blob_file)->vtbl->Close)((struct Gio *)
                                                          &blob_file)) {
      fprintf(stderr, "Error while closing \"%s\".\n", blob_library_file);
    }
    (*((struct Gio *) &blob_file)->vtbl->Dtor)((struct Gio *) &blob_file);
    if (verbosity) {
      gprintf((struct Gio *) &gout, "printing post-IRT NaClApp details\n");
      NaClAppPrintDetails(nap, (struct Gio *) &gout);
    }
  }

  /*
   * Print out a marker for scripts to use to mark the start of app
   * output.
   */
  NaClLog(1, "NACL: Application output follows\n");

  /*
   * Chroot() ourselves.  Based on agl's chrome implementation.
   *
   * TODO(mseaborn): This enables a SUID-based Linux outer sandbox,
   * but it is not used now.  When we do have an outer sandbox for
   * standalone sel_ldr, we should enable it earlier on, and merge it
   * with NaClEnableOuterSandbox().
   */
  sandbox_fd_string = getenv(NACL_SANDBOX_CHROOT_FD);
  if (NULL != sandbox_fd_string) {
    static const char kChrootMe = 'C';
    static const char kChrootSuccess = 'O';

    char* endptr;
    char reply;
    int fd;
    long fd_long;
    errno = 0;  /* To distinguish between success/failure after call */

    fd_long = strtol(sandbox_fd_string, &endptr, 10);

    NaClLog(1, "Chrooting the NaCl module\n");
    if ((ERANGE == errno && (LONG_MAX == fd_long || LONG_MIN == fd_long)) ||
        (0 != errno && 0 == fd_long)) {
      perror("strtol");
      exit(1);
    }
    if (endptr == sandbox_fd_string) {
      fprintf(stderr, "Could not initialize sandbox fd: No digits found\n");
      exit(1);
    }
    if (*endptr) {
      fprintf(stderr, "Could not initialize sandbox fd: Extra digits\n");
      exit(1);
    }
    fd = fd_long;

    /*
     * TODO(neha): When we're more merged with chrome, use HANDLE_EINTR()
     */
    if (write(fd, &kChrootMe, 1) != 1) {
      fprintf(stderr, "Cound not signal sandbox to chroot()\n");
      exit(1);
    }

    /*
     * TODO(neha): When we're more merged with chrome, use HANDLE_EINTR()
     */
    if (read(fd, &reply, 1) != 1) {
      fprintf(stderr, "Could not get response to chroot() from sandbox\n");
      exit(1);
    }
    if (kChrootSuccess != reply) {
      fprintf(stderr, "%s\n", &reply);
      fprintf(stderr, "Reply not correct\n");
      exit(1);
    }
  }

  /*
   * Make sure all the file buffers are flushed before entering
   * the application code.
   */
  fflush((FILE *) NULL);

  /*
   * error reporting done; can quit now if there was an error earlier.
   */
  if (LOAD_OK != errcode) {
    NaClLog(4, "Not running app code since errcode is %s (%d)\n",
            NaClErrorString(errcode),
            errcode);
    goto done;
  }

  if (!DynArraySet(&env_vars, env_vars.num_entries, NULL)) {
    NaClLog(LOG_FATAL, "Adding env_vars NULL terminator failed\n");
  }

  NaClEnvCleanserCtor(&env_cleanser, 0);
  if (!NaClEnvCleanserInit(&env_cleanser, envp,
          (char const *const *)env_vars.ptr_array)) {
    NaClLog(LOG_FATAL, "Failed to initialise env cleanser\n");
  }

  /*
   * d'b: provide nexe with given in manifest command line
   */
  if(nexe_argc == 1)
  {
  	nexe_argc = argc - optind;
  	nexe_argv = argv + optind;
  }

  /* set user space to max_mem */
  PreallocateUserMemory(nap);

  if (!NaClCreateMainThread(nap, nexe_argc, nexe_argv,
  		NaClEnvCleanserEnvironment(&env_cleanser)))
  {
  	fprintf(stderr, "creating main thread failed\n");
    goto done;
  }
  /* d'b end */

  NaClEnvCleanserDtor(&env_cleanser);

  NaClPerfCounterMark(&time_all_main, "CreateMainThread");
  NaClPerfCounterIntervalLast(&time_all_main);
  DynArrayDtor(&env_vars);

  ret_code = NaClWaitForMainThreadToExit(nap);
  NaClPerfCounterMark(&time_all_main, "WaitForMainThread");
  NaClPerfCounterIntervalLast(&time_all_main);

  NaClPerfCounterMark(&time_all_main, "SelMainEnd");
  NaClPerfCounterIntervalTotal(&time_all_main);

  /* d'b
   * manifest finalization
   */
  if(nap->manifest)
  {
    FILE *f = NULL;
    char *name = nap->manifest->system_setup->report;

    /* open report file */
    if ((f = fopen(name, "w")) == NULL)
    {
      NaClLog(LOG_ERROR, "cannot open report manifest = %s\n", name);
      goto done;
    }

    /* generate report, "manifest" reused for report, fix it ### */
    SetupReportSettings(nap);
    nap->manifest->report->ret_code = 0;
    nap->manifest->report->user_ret_code = ret_code;
    AnswerManifestPut(nap, manifest);

    /* write it and free resources */
    fwrite(manifest, 1, strlen(manifest), f);
    fclose(f);

    /* trim user_log */
    // refactor it to premap with name: UnmountChannel(nap, channel) to avoid truncate()
    if (nap->manifest->user_setup->channels[LogChannel].buffer)
    {
      char *p = (char*) NaClUserToSys(nap, (uint32_t)nap->manifest->user_setup->channels[LogChannel].buffer);
      off_t size = strlen(p);
      munmap(p, nap->manifest->user_setup->channels[LogChannel].bsize);
      truncate(GetValueByKey(nap, "UserLog"), size);
    }
  }
  /* d'b end */

  /*
   * exit_group or equiv kills any still running threads while module
   * addr space is still valid.  otherwise we'd have to kill threads
   * before we clean up the address space.
   */
  NaClExit(ret_code);

 done:
  fflush(stdout);

  if (verbosity) {
    gprintf((struct Gio *) &gout, "exiting -- printing NaClApp details\n");
    NaClAppPrintDetails(nap, (struct Gio *) &gout);

    printf("Dumping vmmap.\n"); fflush(stdout);
    PrintVmmap(nap);
    fflush(stdout);
  }

 done_file_dtor:
  if (verbosity > 0) {
    printf("Done.\n");
  }
  fflush(stdout);

  if (handle_signals) NaClSignalHandlerFini();
  NaClAllModulesFini();

  NaClExit(ret_code);

  /* Unreachable, but having the return prevents a compiler error. */
  return ret_code;
}
