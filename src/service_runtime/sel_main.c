/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * ZeroVM main. still under construction. there is a lot of things to remove
 * and to refactor. also the code needs to be polished
 * todo: imc related stuff marked for removal
 */
#include <string.h>
#include <assert.h>
#include <setjmp.h> /* d'b: need for trap() exit */

#include "include/portability_io.h"
#include "src/utils/tools.h"
#include "src/gio/gio.h"
#include "src/platform/nacl_exit.h"
#include "src/fault_injection/fault_injection.h"
#include "src/perf_counter/nacl_perf_counter.h"
#include "src/service_runtime/nacl_all_modules.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_signal.h"
#include "src/service_runtime/include/sys/mman.h" /* d'b */
#include "src/manifest/manifest_parser.h" /* d'b. todo: move to initializer */
#include "src/manifest/manifest_setup.h" /* d'b. todo: move to initializer */
#include "src/manifest/trap.h" /* d'b */
#include "src/manifest/mount_channel.h" /* d'b. todo: move to manifest_setup */
#include "src/service_runtime/sel_qualify.h"

/* todo(NETWORKING): move it channels constructors */
/*YaroslavLitvinov*/
#ifdef NETWORKING
# include "src/networking/zvm_netw.h"
# include "src/networking/zmq_netw.h" /* SockCapability */
# define ZVM_DB_NAME "zvm_netw.db"
#endif
/* end */

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

static void VmentryPrinter(void *state, struct NaClVmmapEntry *vmep)
{
  UNREFERENCED_PARAMETER(state);
  printf("page num 0x%06x\n", (uint32_t) vmep->page_num);
  printf("num pages %d\n", (uint32_t) vmep->npages);
  printf("prot bits %x\n", vmep->prot);
  fflush(stdout);
}

static void PrintVmmap(struct NaClApp *nap)
{
  printf("In PrintVmmap\n");
  fflush(stdout);
  NaClVmmapVisit(&nap->mem_map, VmentryPrinter, NULL);
}

int ImportModeMap(char opt)
{
  switch(opt)
  {
    case 'h':
      return O_RDWR;
    case 'r':
      return O_RDONLY;
    case 'w':
      return O_WRONLY;
  }

  fprintf(stderr, "option %c not understood as a host descriptor import mode\n", opt);
  exit(1);
}

/* parse given command line and initialize NaClApp object */
static void ParseCommandLine(struct NaClApp *nap, int argc, char **argv)
{
  int opt;
  struct redir *entry;
  char *rest;
  int i;
  char *manifest_name = NULL;
  int debug_mode_ignore_validator = 0;
  int enable_debug_stub = 0;
  struct redir *redir_queue = NULL;
  struct redir **redir_qend = &redir_queue;
  int stub_out_mode = 0;

  nap->handle_signals = 0;
  nap->verbosity = NaClLogGetVerbosity();
  nap->skip_qualification = 0;
  nap->fuzzing_quit_after_load = 0;

  /* note: in a future zerovm command line will be reduced */
  while((opt = getopt(argc, argv, "+cFgh:i:Il:QDZr:sSv:w:X:M:")) != -1)
  {
    switch(opt)
    {
      case 'M':
        manifest_name = optarg;
        break;
      case 'c':
        ++debug_mode_ignore_validator;
        break;
      case 'F':
        nap->fuzzing_quit_after_load = 1;
        break;
      case 'g':
        nap->handle_signals = 1;
        enable_debug_stub = 1;
        break;
      case 'S':
        nap->handle_signals = 1;
        break;
      case 'h':
      case 'r':
      case 'w':
        /* import host descriptor */
        entry = malloc(sizeof *entry);
        COND_ABORT(NULL == entry, "No memory for redirection queue");
        entry->next = NULL;
        entry->nacl_desc = strtol(optarg, &rest, 0);
        entry->tag = HOST_DESC;
        entry->u.host.d = strtol(rest + 1, (char **) 0, 0);
        entry->u.host.mode = ImportModeMap(opt);
        *redir_qend = entry;
        redir_qend = &entry->next;
        break;
      case 'i':
        /* import IMC handle */
        entry = malloc(sizeof *entry);
        COND_ABORT(NULL == entry, "No memory for redirection queue");
        entry->next = NULL;
        entry->nacl_desc = strtol(optarg, &rest, 0);
        entry->tag = IMC_DESC;
        entry->u.handle = (NaClHandle) strtol(rest + 1, (char **) 0, 0);
        *redir_qend = entry;
        redir_qend = &entry->next;
        break;
        /* case 'r':  with 'h' and 'w' above */
      case 'v':
        i = atoi(optarg);
        i = nap->verbosity = i < 1 ? 0 : i;
        while(i--)
          NaClLogIncrVerbosity();
        break;
        /* case 'w':  with 'h' and 'r' above */
      case 'Q':
        fprintf(stderr, "PLATFORM QUALIFICATION DISABLED BY -Q - "
                "Native Client's sandbox will be unreliable!\n");
        nap->skip_qualification = 1;
        break;
      case 's':
        stub_out_mode = 1;
        break;
      case 'Z':
        nap->fixed_feature_cpu_mode = 1;
        break;
      case 'D':
        nap->enable_dfa_validator = 1;
        fprintf(stderr, "DANGER! USING THE UNSTABLE DFA VALIDATOR!\n");
        break;
      default:
        fprintf(stderr, "ERROR: unknown option: [%c]\n\n", opt);
        COND_ABORT(1, HELP_SCREEN);
        break;
    }
  }

  /* show validator mode */
  if(debug_mode_ignore_validator == 1)
    fprintf(stderr, "DEBUG MODE ENABLED (ignore validator)\n");
  else if(debug_mode_ignore_validator > 1)
    fprintf(stderr, "DEBUG MODE ENABLED (skip validator)\n");

  /* show zerovm command line */
  if(nap->verbosity)
  {
    fprintf(stderr, "zerovm argument list:\n");
    for(i = 0; i < argc; ++i)
      fprintf(stderr, "%s ", argv[i]);
    fprintf(stderr, "\n");
  }

  /* parse manifest file specified in cmdline */
  COND_ABORT(manifest_name == NULL, HELP_SCREEN);
  COND_ABORT(ManifestCtor(manifest_name), "Invalid manifest file");

  /* set available nap and manifest fields */
  assert(nap->system_manifest != NULL);
  nap->ignore_validator_result = (debug_mode_ignore_validator > 0);
  nap->skip_validator = (debug_mode_ignore_validator > 1);
  nap->validator_stub_out_mode = stub_out_mode;
  nap->enable_debug_stub = enable_debug_stub;
  nap->user_side_flag = 0; /* we are in the trusted code */
  nap->system_manifest->nexe = GetValueByKey("Nexe");
  nap->system_manifest->blob = GetValueByKey("Blob");
  nap->system_manifest->log = GetValueByKey("Log");
  gnap = nap;
  syscallback = 0;
}

/*
 * set everything in the given NaClApp object to 0s and NULLs
 * so we can assert() nap and nap pointers before usage
 * note: always return LOAD_OK because we want to put the function inside assert()
 */
#ifdef DEBUG
int ResetNap(struct NaClApp *nap)
{
  memset(nap, 0, sizeof(*nap));

  /* since NULL isn't neccessery 0 set it explicitly */
  nap->debug_stub_callbacks = NULL;
  nap->dynamic_page_bitmap = NULL;
  nap->dynamic_regions = NULL;
  nap->effp = NULL;
  nap->host_manifest = NULL;
  nap->signal_stack = NULL;
  nap->syscall_args = NULL;
  nap->syscall_table = NULL;
  nap->system_manifest = NULL;
  nap->text_shm = NULL;

  return LOAD_OK;
}
#endif

int main(int argc, char **argv)
{
  struct NaClApp                state, *nap = &state;
  struct SystemManifest         sys_mft;
  struct HostManifest           host_mft;
  NaClErrorCode                 errcode = LOAD_INTERNAL;
  struct GioFile                gout;
  struct GioMemoryFileSnapshot  blob_file;
  struct GioMemoryFileSnapshot  main_file;
  struct NaClPerfCounter        time_all_main;

  /* todo(d'b): make zerovm retcode. or make it always 0 and eliminate from the code */
  int                           ret_code = 1;

  /* (re)set all (debug) or some of nap fields */
  assert(ResetNap(nap) == LOAD_OK);
  nap->system_manifest = &sys_mft;
  nap->host_manifest = &host_mft;

  /* @IGNORE_LINES_FOR_CODE_HYGIENE[1] */
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

  NaClAllModulesInit();
  NaClPerfCounterCtor(&time_all_main, "SelMain");
  fflush((FILE *) NULL);
  COND_ABORT(!GioFileRefCtor(&gout, stdout), "Could not create general standard output channel");
  ParseCommandLine(nap, argc, argv);

	/*
	 * change stdout/stderr to log file now, so that subsequent error
	 * messages will go there.  unfortunately, error messages that
	 * result from getopt processing -- usually out-of-memory, which
	 * shouldn't happen -- won't show up.
	 */
  if(NULL != nap->system_manifest->log) NaClLogSetFile(nap->system_manifest->log);

  /* todo(d'b): remove it */
  NaClLogGetGio();

  /* the dyn_array constructor 1st call */
  COND_ABORT(NaClAppCtor(nap) == 0, "Error while constructing app state");
	errcode = LOAD_OK;

	/* We use the signal handler to verify a signal took place. */
	NaClSignalHandlerInit();
	if (nap->skip_qualification == 0)
	{
		NaClErrorCode pq_error = NACL_FI_VAL("pq", NaClErrorCode, NaClRunSelQualificationTests());
		if (LOAD_OK != pq_error)
		{
			errcode = pq_error;
			nap->module_load_status = pq_error;
			fprintf(stderr, "Error while loading \"%s\": %s\n",
					NULL != nap->system_manifest->nexe ?
					nap->system_manifest->nexe : "(no file, to-be-supplied-via-RPC)",
					NaClErrorString(errcode));
		}
	}

  /* Remove the signal handler if we are not using it. */
	if (nap->handle_signals == 0)
	{
		NaClSignalHandlerFini();
		NaClSignalAssertNoHandlers(); /* Sanity check. */
	}

#define PERF_CNT(str)\
	NaClPerfCounterMark(&time_all_main, str);\
  NaClPerfCounterIntervalLast(&time_all_main);

#define READ_MODULE(snapshot, name, perf) \
  do {\
    if(0 == GioMemoryFileSnapshotCtor(snapshot, name))\
    {\
      perror("sel_main");\
      fprintf(stderr, "Cannot open \"%s\".\n", name);\
      exit(1);\
    }\
    PERF_CNT(perf);\
  } while(0)

#define VALIDATE_MODULE(snapshot, name) \
  do {\
    NaClLog(2, "Loading nacl file %s (non-RPC)\n", name);\
    errcode = NaClAppLoadFile((struct Gio *) snapshot, nap);\
    if (LOAD_OK != errcode)\
    {\
      fprintf(stderr, "Error while loading \"%s\": %s\n", name,\
              NaClErrorString(errcode));\
      fprintf(stderr, ("Using the wrong type of nexe (nacl-x86-32"\
              " on an x86-64 or vice versa)\nor a corrupt nexe file may be"\
              " responsible for this error.\n"));\
    }\
  } while(0)

	/*
	 * todo(d'b): find out how to compile a blob library. pay attention
	 * that we change the blob nature: it is an untrusted code now.
	 * the blob library differ from the nexe only by 1 thing: order
	 * of initialization. blob will take control before nexe
	 *
	 * another important thing: how to load blob and nexe. they
	 * will share 4gb user space
	 */

  /* read the nexe and (if specified, the blob) into the memory */
  /* ###
   * todo(d'b): blob loading temporary disabled
   * probably blob will be completely removed
   */
	UNREFERENCED_PARAMETER(blob_file);
//  if(NULL != nap->system_manifest->blob)
//    READ_MODULE(&blob_file, nap->system_manifest->blob, "SnapshotBlob");
  READ_MODULE(&main_file, nap->system_manifest->nexe, "SnapshotNaclFile");

  /* validate untrusted code: the nexe and (if specified, the blob) */
  if (LOAD_OK == errcode)
  {
    /* ###
     * todo(d'b): blob loading temporary disabled
     * probably blob will be completely removed
     */
//    if(NULL != nap->system_manifest->blob)
//      VALIDATE_MODULE(&blob_file, nap->system_manifest->blob);

    VALIDATE_MODULE(&main_file, nap->system_manifest->nexe);

    PERF_CNT("AppLoadEnd");
    nap->module_load_status = errcode;
  }

  if(-1 == (*((struct Gio *) &main_file)->vtbl->Close)((struct Gio *) &main_file))
  {
    fprintf(stderr, "Error while closing \"%s\".\n", nap->system_manifest->nexe);
  }

  (*((struct Gio *) &main_file)->vtbl->Dtor)((struct Gio *) &main_file);
  if(nap->fuzzing_quit_after_load) exit(0);

  /*
   * construct system and host manifests
   * todo(d'b): move it to "src/platform/platform_init.c" chain. problems to solve:
   * - channels construction needs initialized nacl descriptors (it is dyn_array)
   * - "memory chunk" needs initialized memory manager (user stack, text, data e.t.c)
   */
  SystemManifestCtor(nap); /* needs dyn_array initialized */
  HostManifestCtor(nap); /* needs dyn_array initialized */
  assert(nap->system_manifest != NULL); /* just in case. can be removed */

  /* todo(NETWORKING): move it to initializers. rewrite hardcoded part */
  /*YaroslavLitvinov*/
#ifdef NETWORKING
  if(GetValueByKey("Networking") != NULL)
  {
    /* check before networking initialization */
    assert(nap != NULL);
    assert(nap->system_manifest != NULL);

    if(nap->system_manifest->cmd_line == NULL || nap->system_manifest->cmd_line_size < 3)
      NaClLog(LOG_FATAL, "NETWORKING defined but command line parameters not valid\n");

    do { /* prevent c90 warning */
      /* get node generic name */
      char *netw_nodename = nap->system_manifest->cmd_line[2];

      /* get node id */
      int nodeid = atoi(nap->system_manifest->cmd_line[1]);
      if(netw_nodename != NULL)
      {
        int err = init_zvm_networking(zmq_netw_interface_implementation(),
                                      ZVM_DB_NAME, netw_nodename, nodeid);
        if (LOAD_OK != err)
          NaClLog(LOG_FATAL, "init_zvm_networking err = %d, nodename = %s\n",
                  err, netw_nodename);
      }
    } while(0);
  }
#endif
  /* end */

  /*
   * todo(d'b): we gonna load the blob using the nexe loader (and
   * it has been done in the code above)
   */

//  /* load blob library */
//  if(NULL != nap->system_manifest->blob)
//  {
//    if(LOAD_OK == errcode)
//    {
//      NaClLog(2, "Loading blob file %s\n", nap->system_manifest->blob);
//      errcode = NaClAppLoadFileDynamically(nap, (struct Gio *) &blob_file);
//      if(LOAD_OK != errcode)
//      {
//        fprintf(stderr, "Error while loading \"%s\": %s\n", nap->system_manifest->blob,
//                NaClErrorString(errcode));
//      }
//      PERF_CNT("BlobLoaded");
//    }
//
//    if(-1 == (*((struct Gio *) &blob_file)->vtbl->Close)((struct Gio *) &blob_file))
//    {
//      fprintf(stderr, "Error while closing \"%s\".\n", nap->system_manifest->blob);
//    }
//    (*((struct Gio *) &blob_file)->vtbl->Dtor)((struct Gio *) &blob_file);
//    if(nap->verbosity)
//    {
//      gprintf((struct Gio *) &gout, "printing post-IRT NaClApp details\n");
//      NaClAppPrintDetails(nap, (struct Gio *) &gout);
//    }
//  }

  /* error reporting done; can quit now if there was an error earlier */
  if (LOAD_OK != errcode) {
    NaClLog(4, "Not running app code since errcode is %s (%d)\n",
            NaClErrorString(errcode), errcode);
    goto done;
  }

  PERF_CNT("CreateMainThread");

  /* Make sure all the file buffers are flushed before entering the nexe */
  fflush((FILE *) NULL);

  /* set user code trap() exit location */
  if((ret_code = setjmp(user_exit)) == 0)
  {
    /* pass control to the user code */
    if(!NaClCreateMainThread(nap, nap->system_manifest->cmd_line_size,
                             nap->system_manifest->cmd_line, NULL))
    {
      fprintf(stderr, "creating main thread failed\n");
      goto done;
    }
  }
  PERF_CNT("WaitForMainThread");
  PERF_CNT("SelMainEnd");

  /* todo(NETWORKING): move it to mount_channel.c (destructor) */
  /*YaroslavLitvinov*/
#ifdef NETWORKING
  if(GetValueByKey("Networking") != NULL)
  {
    int err = term_zvm_networking();
    if (LOAD_OK != err)
      NaClLog(LOG_FATAL, "term_zvm_networking err = %d", err);
  }
#endif
  /* end */

  /* report to host. call destructors */
  SystemManifestDtor(nap);
  HostManifestDtor(nap);

  /* todo(): replace with ret_code when zerovm error codes will be ready */
  NaClExit(0);

 done:
  if(nap->verbosity)
  {
    gprintf((struct Gio *) &gout, "exiting -- printing NaClApp details\n");
    NaClAppPrintDetails(nap, (struct Gio *) &gout);
    printf("Dumping vmmap.\n");
    PrintVmmap(nap);
  }

  if(nap->verbosity) printf("Done.\n");
  if (nap->handle_signals) NaClSignalHandlerFini();
  NaClAllModulesFini();
  NaClExit(ret_code);

  /* Unreachable, but having the return prevents a compiler error. */
  return ret_code;
}
