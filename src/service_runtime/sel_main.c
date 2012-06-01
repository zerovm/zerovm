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
#include <setjmp.h> /* d'b: need for trap() exit */

#include "include/portability_io.h"
#include "src/gio/gio.h"
#include "src/platform/nacl_exit.h"
#include "src/fault_injection/fault_injection.h"
#include "src/perf_counter/nacl_perf_counter.h"
#include "src/service_runtime/nacl_all_modules.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_signal.h"
#include "src/service_runtime/include/sys/mman.h" /* d'b */
#include "src/manifest/manifest_parser.h" /* d'b */
#include "src/manifest/manifest_setup.h" /* d'b */
#include "src/manifest/trap.h" /* d'b */
#include "src/manifest/mount_channel.h" /* d'b */
#include "src/service_runtime/sel_qualify.h"
#include "zmq.h"

/*YaroslavLitvinov*/
#ifdef NETWORKING
//extern "C"{
	#include "src/networking/sqluse_srv.h"
	#include "src/networking/zmq_netw.h"
//}
	#define ZVM_DB_NAME "zvm_netw.db"
#endif

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
}

static void PrintUsage() {
  /* NOTE: this is broken up into multiple statements to work around
           the constant string size limit */
  fprintf(stderr,
          "Usage: sel_ldr [-M manifest_file] [-h d:D] [-r d:D]\n"
          "               [-w d:D] [-i d:D] [-v d] [-cFgIsQZD]\n\n"
          " -h\n"
          " -r\n"
          " -w associate a host POSIX descriptor D with app desc d\n"
          "    that was opened in O_RDWR, O_RDONLY, and O_WRONLY modes\n"
          "    respectively\n"
          " -i associates an IMC handle D with app desc d\n"
          " -v <level> verbosity\n\n"
          " (testing flags)\n"
          " -c ignore validator! dangerous! Repeating this option twice skips\n"
          "    validation completely.\n"
          " -F fuzz testing; quit after loading NaCl app\n"
          " -S enable signal handling.\n"
          " -g enable gdb debug stub.\n"
          " -l <file>  write log output to the given file\n"
          " -s safely stub out non-validating instructions\n"
          " -Q disable platform qualification (dangerous!)\n"
		      " -M <file> load settings from manifest\n"
          " -Z use fixed feature x86 CPU mode\n"
          " -D enable the UNSTABLE dfa validator\n"
          );  /* easier to add new flags/lines */
}

/*
 * parse given command line and initialize NaClApp object
 * return 0 if success, non-zero error code if failed
 */
int ParseCommandLine(int argc, char **argv, struct NaClApp *nap)
{
  int opt;
  struct redir *entry;
  char *rest;
  int i;
  int nexe_argc = 1;
  char **nexe_argv;
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
  //YaroslavLitvinov: Fixed segmentation fault, if manifest file not found
  nap->manifest = 0;

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
        if(NULL == entry)
        {
          fprintf(stderr, "No memory for redirection queue\n");
          return ERR_CODE;
        }
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
        if(NULL == entry)
        {
          fprintf(stderr, "No memory for redirection queue\n");
          return ERR_CODE;
        }
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
        PrintUsage();
        return ERR_CODE;
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

  /* process manifest file specified in cmdline */
  if (manifest_name != NULL)
  {
    int32_t size;
    if(!parse_manifest(manifest_name, nap))
    {
      fprintf(stderr, "Invalid manifest file \"%s\".\n", manifest_name);
      return ERR_CODE;
    }

    /* initialize user policy, zerovm settings */
    SetupUserPolicy(nap);
    SetupSystemPolicy(nap);

    /* construct nexe command line from manifest (add nexe name as argv[0]) */
    COND_ABORT(!(nexe_argv = malloc(128 * sizeof(char*))),
        "cannot allocate memory for nexe command line\n");
    nexe_argv[0] = "_";
    nexe_argv[nexe_argc] = strtok(get_value_by_key(nap, "CommandLine"), " \t");
    while(nexe_argv[nexe_argc])
      nexe_argv[++nexe_argc] = strtok(NULL, " \t");
    nap->manifest->system_setup->cmd_line = nexe_argv;
    nap->manifest->system_setup->cmd_line_size = nexe_argc;

    /* check for limits given in manifest */
    COND_ABORT(nap->manifest->system_setup->version == NULL,
        "manifest version is not provided\n");
    COND_ABORT(strcmp(nap->manifest->system_setup->version, MANIFEST_VERSION),
        "wrong manifest version\n");
    if((size = GetFileSize(nap->manifest->system_setup->nexe)) < 0)
    {
      fprintf(stderr, "%s not found\n", nap->manifest->system_setup->nexe);
      return ERR_CODE;
    }
    if(nap->manifest->system_setup->nexe_max)
      COND_ABORT(nap->manifest->system_setup->nexe_max < size, "nexe file is greater then alowed\n");
  }
  else /* manifest file is not provided */
  {
    PrintUsage();
    return ERR_CODE;
  }

  nap->ignore_validator_result = (debug_mode_ignore_validator > 0);
  nap->skip_validator = (debug_mode_ignore_validator > 1);
  nap->validator_stub_out_mode = stub_out_mode;
  nap->enable_debug_stub = enable_debug_stub;

  /* check if nexe is given */
  if(NULL == nap->manifest->system_setup->nexe)
  {
    PrintUsage();
    return ERR_CODE;
  }
  return OK_CODE;
}

int main(int argc, char **argv)
{
  struct NaClApp                state, *nap = &state;
  NaClErrorCode                 errcode = LOAD_INTERNAL;
  struct GioFile                gout;
  struct GioFile                *log_gio;
  struct GioMemoryFileSnapshot  blob_file;
  struct GioMemoryFileSnapshot  main_file;
  struct NaClPerfCounter        time_all_main;
  int                           ret_code = 1;
  char                          manifest[MAX_MANIFEST_LEN];
  int                           err = 0;

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
  COND_ABORT(!GioFileRefCtor(&gout, stdout), "Could not create general standard output channel\n");
  COND_ABORT(ParseCommandLine(argc, argv, nap), "command line parse error\n");

	/*
	 * change stdout/stderr to log file now, so that subsequent error
	 * messages will go there.  unfortunately, error messages that
	 * result from getopt processing -- usually out-of-memory, which
	 * shouldn't happen -- won't show up.
	 */
	if (NULL != nap->manifest && NULL != nap->manifest->system_setup->log)
	  NaClLogSetFile(nap->manifest->system_setup->log);

  /*
   * NB: the following cast is okay since we only ever permit GioFile
   * objects to be used -- NaClLogModuleInit and NaClLogSetFile both
   * can only assign the log output to a file.  If neither were
   * called, logging goes to stderr.
   */
  log_gio = (struct GioFile*) NaClLogGetGio();

  COND_ABORT(!NaClAppCtor(nap), "Error while constructing app state\n");
	errcode = LOAD_OK;

	/* We use the signal handler to verify a signal took place. */
	NaClSignalHandlerInit();
	if (!nap->skip_qualification)
	{
		NaClErrorCode pq_error = NACL_FI_VAL("pq", NaClErrorCode, NaClRunSelQualificationTests());
		if (LOAD_OK != pq_error)
		{
			errcode = pq_error;
			nap->module_load_status = pq_error;
			fprintf(stderr, "Error while loading \"%s\": %s\n",
					NULL != nap->manifest->system_setup->nexe ?
					nap->manifest->system_setup->nexe : "(no file, to-be-supplied-via-RPC)",
					NaClErrorString(errcode));
		}
	}

  /* Remove the signal handler if we are not using it. */
	if (!nap->handle_signals)
	{
		NaClSignalHandlerFini();
		NaClSignalAssertNoHandlers(); /* Sanity check. */
	}

  /* Open (not load) both files nexe and blob. only need for "fuzzy load". can be removed */
#define PERF_CNT(str)\
	NaClPerfCounterMark(&time_all_main, str);\
  NaClPerfCounterIntervalLast(&time_all_main);

  if(NULL != nap->manifest->system_setup->blob)
  {
    if(0 == GioMemoryFileSnapshotCtor(&blob_file, nap->manifest->system_setup->blob))
    {
      perror("sel_main");
      fprintf(stderr, "Cannot open \"%s\".\n", nap->manifest->system_setup->blob);
      exit(1);
    }
    PERF_CNT("SnapshotBlob");
  }

  if (0 == GioMemoryFileSnapshotCtor(&main_file, nap->manifest->system_setup->nexe))
  {
    perror("sel_main");
    fprintf(stderr, "Cannot open \"%s\".\n", nap->manifest->system_setup->nexe);
    exit(1);
  }
  PERF_CNT("SnapshotNaclFile");

  if (LOAD_OK == errcode)
  {
    NaClLog(2, "Loading nacl file %s (non-RPC)\n", nap->manifest->system_setup->nexe);
    errcode = NaClAppLoadFile((struct Gio *) &main_file, nap);
    if (LOAD_OK != errcode)
    {
      fprintf(stderr, "Error while loading \"%s\": %s\n", nap->manifest->system_setup->nexe,
              NaClErrorString(errcode));
      fprintf(stderr, ("Using the wrong type of nexe (nacl-x86-32"
              " on an x86-64 or vice versa)\nor a corrupt nexe file may be"
              " responsible for this error.\n"));
    }
    PERF_CNT("AppLoadEnd");
    nap->module_load_status = errcode;
  }

  if(-1 == (*((struct Gio *) &main_file)->vtbl->Close)((struct Gio *) &main_file))
  {
    fprintf(stderr, "Error while closing \"%s\".\n", nap->manifest->system_setup->nexe);
  }

  (*((struct Gio *) &main_file)->vtbl->Dtor)((struct Gio *) &main_file);
  if(nap->fuzzing_quit_after_load) exit(0);

   /*YaroslavLitvinov*/
#ifdef NETWORKING
  nap->db_records = malloc(sizeof(struct db_records_t));
  memset(nap->db_records, '\0', sizeof(struct db_records_t));
  if ( nap->manifest && nap->manifest->system_setup->cmd_line && nap->manifest->system_setup->cmd_line_size >= 3 ){
	  /*get client name*/
	  char *netw_nodename = nap->manifest->system_setup->cmd_line[2];
	  /*set client id whose configuration should load*/
	  nap->db_records->cid = atoi(nap->manifest->system_setup->cmd_line[1]);
	  if ( netw_nodename ){
		  uint64_t db_size = GetFileSize(ZVM_DB_NAME);
		  if ( -1 != db_size && 0 != db_size ){
			  NaClLog(LOG_INFO, "reading database = %s, cid=%d, nodename=%s\n", ZVM_DB_NAME, nap->db_records->cid, netw_nodename);
			  err = get_all_records_from_dbtable(ZVM_DB_NAME, netw_nodename, nap->db_records);
			  if ( err ){
				  NaClLog(LOG_ERROR, "database %s read error= %d\n", ZVM_DB_NAME, err);
			  }else{
				  nap->zmq_pool = malloc(sizeof(struct zeromq_pool));
				  init_zeromq_pool(nap->zmq_pool);
				  open_all_comm_files(nap->zmq_pool, nap->db_records);
			  }
		  }
		  else{
			  NaClLog(LOG_ERROR, "NETWORKING defined, db does not exist or empty\n");
		  }
	  }
	  else{
		  NaClLog(LOG_ERROR, "NETWORKING defined, but NULL nodename??\n");
	  }
  }
  else{
	  NaClLog(LOG_ERROR, "NETWORKING defined but command line parameters not valid\n");
  }
#endif


  /* construct each mentioned in manifest channel and mount it */
  if(nap->manifest)
  {
    enum ChannelType ch;
    for(ch = InputChannel; ch < CHANNELS_COUNT; ++ch)
    {
      if(ConstructChannel(nap, ch)) continue;
      MountChannel(nap, ch);
    }
  }

  /* load blob library */
  if(NULL != nap->manifest->system_setup->blob)
  {
    if(LOAD_OK == errcode)
    {
      NaClLog(2, "Loading blob file %s\n", nap->manifest->system_setup->blob);
      errcode = NaClAppLoadFileDynamically(nap, (struct Gio *) &blob_file);
      if(LOAD_OK != errcode)
      {
        fprintf(stderr, "Error while loading \"%s\": %s\n", nap->manifest->system_setup->blob,
                NaClErrorString(errcode));
      }
      PERF_CNT("BlobLoaded");
    }

    if(-1 == (*((struct Gio *) &blob_file)->vtbl->Close)((struct Gio *) &blob_file))
    {
      fprintf(stderr, "Error while closing \"%s\".\n", nap->manifest->system_setup->blob);
    }
    (*((struct Gio *) &blob_file)->vtbl->Dtor)((struct Gio *) &blob_file);
    if(nap->verbosity)
    {
      gprintf((struct Gio *) &gout, "printing post-IRT NaClApp details\n");
      NaClAppPrintDetails(nap, (struct Gio *) &gout);
    }
  }

  /* error reporting done; can quit now if there was an error earlier */
  if (LOAD_OK != errcode) {
    NaClLog(4, "Not running app code since errcode is %s (%d)\n",
            NaClErrorString(errcode), errcode);
    goto done;
  }

  /* set user space to max_mem if specified in the manifest */
  PreallocateUserMemory(nap);

  PERF_CNT("CreateMainThread");

  /* Make sure all the file buffers are flushed before entering the nexe */
  fflush((FILE *) NULL);

  /* set user code trap() exit location */
  if((ret_code = setjmp(user_exit)) == 0)
  {
    /* pass control to the user code */
    if(!NaClCreateMainThread(nap, nap->manifest->system_setup->cmd_line_size,
                             nap->manifest->system_setup->cmd_line, NULL))
    {
      fprintf(stderr, "creating main thread failed\n");
      goto done;
    }
  }
  PauseCpuClock(nap);
  PERF_CNT("WaitForMainThread");
  PERF_CNT("SelMainEnd");

  /* manifest finalization */
  if(nap->manifest)
  {
    FILE *f = NULL;
    char *name = nap->manifest->system_setup->report;
    struct PreOpenedFileDesc *log_ch = &nap->manifest->user_setup->channels[LogChannel];

    /* make report if specified in manifest */
    if(nap->manifest->system_setup->report != NULL)
    {
      if((f = fopen(name, "w")) == NULL)
      {
        NaClLog(LOG_ERROR, "cannot open report manifest = %s\n", name);
        goto done;
      }
      /* generate report, "manifest" reused for report, fix it ### */
      SetupReportSettings(nap);
      nap->manifest->report->ret_code = 0;
      nap->manifest->report->user_ret_code = nap->exit_status;
      AnswerManifestPut(nap, manifest);

      /* write it and free resources */
      fwrite(manifest, 1, strlen(manifest), f);
      fclose(f);
    }

    // make it function: UnmountChannel(nap, channel) to avoid truncate()
    /* trim user_log */
    if (log_ch->buffer)
    {
      char *p = (char*) NaClUserToSys(nap, (uint32_t)log_ch->buffer);
      log_ch->fsize = strlen(p);
      munmap(p, log_ch->bsize);
      if(log_ch->fsize) truncate((char*)log_ch->name, log_ch->fsize);
      else remove((char*)log_ch->name);
    }

  }

  /*YaroslavLitvinov*/
#ifdef NETWORKING
  if (nap->zmq_pool && nap->db_records){
	  NaClLog(LOG_ERROR, "1");
	  close_all_comm_files(nap->zmq_pool);
	  NaClLog(LOG_ERROR, "2");
  }
#endif

  NaClExit(ret_code);

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
