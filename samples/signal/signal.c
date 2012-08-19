/* zerovm signal handling test */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#ifdef USER_SIDE
#include "zrt.h"
#endif

int main(int argc, char **argv, char **envp)
{
  int signal = _NSIG;

  if(argc == 2) signal = atoi(argv[1]);
  
  /* since raise() doesn't work from zerovm.. */
  memset((void*)1, 0xff, -1); /* #11 */

#if 0
  /*
   * duplicates were removed: SIGIOT, SIGCLD, SIGPOLL
   */
  switch(signal)
  {
    case SIGHUP: /* Hangup (POSIX).  */
    case SIGINT: /* Interrupt (ANSI).  */
    case SIGQUIT: /* Quit (POSIX).  */
    case SIGILL: /* Illegal instruction (ANSI).  */
    case SIGTRAP: /* Trace trap (POSIX).  */
    case SIGABRT: /* Abort (ANSI).  */
    case SIGBUS: /* BUS error (4.2 BSD).  */
    case SIGFPE: /* Floating-point exception (ANSI).  */
    case SIGKILL: /* Kill, unblockable (POSIX).  */
    case SIGUSR1: /* User-defined signal 1 (POSIX).  */
    case SIGSEGV: /* Segmentation violation (ANSI).  */
    case SIGUSR2: /* User-defined signal 2 (POSIX).  */
    case SIGPIPE: /* Broken pipe (POSIX).  */
    case SIGALRM: /* Alarm clock (POSIX).  */
    case SIGTERM: /* Termination (ANSI).  */
    case SIGSTKFLT: /* Stack fault.  */
    case SIGCHLD: /* Child status has changed (POSIX).  */
    case SIGCONT: /* Continue (POSIX).  */
    case SIGSTOP: /* Stop, unblockable (POSIX).  */
    case SIGTSTP: /* Keyboard stop (POSIX).  */
    case SIGTTIN: /* Background read from tty (POSIX).  */
    case SIGTTOU: /* Background write to tty (POSIX).  */
    case SIGURG: /* Urgent condition on socket (4.2 BSD).  */
    case SIGXCPU: /* CPU limit exceeded (4.2 BSD).  */
    case SIGXFSZ: /* File size limit exceeded (4.2 BSD).  */
    case SIGVTALRM: /* Virtual alarm clock (4.2 BSD).  */
    case SIGPROF: /* Profiling alarm clock (4.2 BSD).  */
    case SIGWINCH: /* Window size change (4.3 BSD, Sun).  */
    case SIGIO: /* I/O now possible (4.2 BSD).  */
    case SIGPWR: /* Power failure restart (System V).  */
    case SIGSYS: /* Bad system call.  */
      printf("emitting signal %d..\n", signal);
      raise(signal);
      printf("exiting..\n");
      break;
    default: /* divide by zero */
      printf("invalid signal number or invalid argument(s) passed\n");
      break;
  }
#endif

  return 0xABAD1DEA;
}
