/*
 * Copyright (c) 2012, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <assert.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include<sys/wait.h>
#include "src/main/report.h"
#include "src/main/accounting.h"
#include "src/platform/signal.h"
#include "src/channels/channel.h"
#include "src/syscalls/daemon.h"

#define TASK_SIZE 0x10000
#define QUEUE_SIZE 0x100
#define CMD_SIZE (sizeof(uint64_t))

static int client = -1;

/*
 * child: get command from inherited command socket. current version can
 * only have one command and therefore the command format will only contain
 * one (pascal) string: 4-bytes length and data of "length" size. the data
 * is manifest (reduced form of it). WARNING: result should be freed
 */
static char *GetCommand()
{
  int len;
  char *cmd = g_malloc0(TASK_SIZE);

  assert(client >= 0);

  ZLOGFAIL(read(client, cmd, CMD_SIZE) < 0, EIO, "%s", strerror(errno));
  len = ToInt(cmd);
  ZLOGFAIL(read(client, cmd, len) < 0, EIO, "%s", strerror(errno));

  return cmd;
}

/* child: update "nap" with the new manifest */
static void UpdateSession(struct Manifest *manifest)
{
  int i;
  char *cmd = GetCommand();
  struct Manifest *tmp = ManifestTextCtor(cmd);

  /* re-initialize signals handling, accounting and the report handle */
  g_free(cmd);
  SignalHandlerFini();
  SignalHandlerInit();
  ResetAccounting();
  ReportMode(3);
  SetReportHandle(client);

  /* copy needful fields from the new manifest */
  manifest->timeout = tmp->timeout;
  manifest->name_server = tmp->name_server;
  manifest->node = tmp->node;
  manifest->job = tmp->job;

  /* check and partially copy channels */
  SortChannels(manifest->channels);
  SortChannels(tmp->channels);

  for(i = 0; i < manifest->channels->len; ++i)
  {
#define CHECK(a) ZLOGFAIL(CH_CH(manifest, i)->a != CH_CH(tmp, i)->a, \
    EFAULT, "difference in %s", CH_CH(manifest, i)->a)

    ZLOGFAIL(strcmp(CH_CH(manifest, i)->alias, CH_CH(tmp, i)->alias) != 0,
        EFAULT, "difference in %s", CH_CH(manifest, i)->alias);
    CHECK(type);
    CHECK(limits[0]);
    CHECK(limits[1]);
    CHECK(limits[2]);
    CHECK(limits[3]);

    CH_CH(manifest, i)->source = CH_CH(tmp, i)->source;
    CH_CH(manifest, i)->tag = CH_CH(tmp, i)->tag;
  }
  ChannelsCtor(manifest);
}

/* daemon: get the next task: return when accept()'ed */
static int Job(int sock)
{
  struct sockaddr_un remote;
  socklen_t len = sizeof remote;
  return accept(sock, &remote, &len);
}

/* convert to the daemon mode */
static void Daemonize(struct NaClApp *nap)
{
  int i;
  struct sigaction sa;
  struct rlimit rl;
  int max_handles;

  /* unmount channels, reset timeout */
  ChannelsDtor(nap->manifest);
  nap->manifest->timeout = 0;
  alarm(0);

  /* Become a session leader to lose controlling TTY */
  setsid();

  /* Ensure future opens won't allocate controlling TTYs */
  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  ZLOGFAIL(sigaction(SIGHUP, &sa, NULL) < 0, EFAULT, "can't ignore SIGHUP");

  /*
   * Change the current working directory to the root so
   * we won't prevent file systems from being unmounted.
   */
  ZLOGFAIL(chdir("/") < 0, EFAULT, "can't change directory to /");

  /* set number of handles to be closed */
  umask(0);
  ZLOGFAIL(getrlimit(RLIMIT_NOFILE, &rl) < 0, EFAULT, "can't get file limit");
  max_handles = MIN(rl.rlim_max, 1024);

  /* Close all open file descriptors */
  for (i = 0; i < max_handles; ++i)
    close(i);

  /* Attach standard channels to /dev/null */
  ZLOGFAIL(open("/dev/null", O_RDWR) != 0, EFAULT, "can't set stdin to /dev/null");
  ZLOGFAIL(dup(0) != 1, EFAULT, "can't set stdout to /dev/null");
  ZLOGFAIL(dup(0) != 2, EFAULT, "can't set stderr to /dev/null");

  /* TODO(d'b): free needless resources */
}

int Daemon(struct NaClApp *nap)
{
  int sock;
  pid_t pid;
  siginfo_t info;
  struct sockaddr_un remote = {AF_UNIX, ""};

  /* can the daemon be started? */
  if(nap->manifest->job == NULL) return -1;
  ZLOGFAIL(GetExitCode(), EFAULT, "broken session");

  /* finalize user session */
  SetDaemonState(1); /* report the daemon mode launched */
  pid = fork();
  if(pid != 0) return 0;
  SetDaemonState(0); /* forked sessions are not in daemon mode */

  /* start the daemon */
  Daemonize(nap);

  /* open the command channel */
  unlink(nap->manifest->job);
  sock = socket(AF_UNIX, SOCK_STREAM, 0);
  strcpy(remote.sun_path, nap->manifest->job);
  ZLOGFAIL(bind(sock, &remote, sizeof remote) < 0, EIO, "%s", strerror(errno));
  ZLOGFAIL(listen(sock, QUEUE_SIZE) < 0, EIO, "%s", strerror(errno));

  for(;;)
  {
    /* get the next job */
    if((client = Job(sock)) < 0)
    {
      ZLOG(LOG_ERROR, "%s", strerror(errno));
      continue;
    }

    /* release waiting child sessions */
    do
      if(waitid(P_ALL, 0, &info, WEXITED | WNOHANG) < 0) break;
    while(info.si_code);

    /* child: update manifest, continue to the trap */
    pid = fork();
    if(pid == 0)
    {
      UpdateSession(nap->manifest);
      break;
    }

    /* daemon: close command socket and wait for a new job */
    close(client);
    ZLOGIF(pid < 0, "fork failed: %s", strerror(errno));
  }

  return -1;
}
