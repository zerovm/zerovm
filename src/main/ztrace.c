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
#include "src/channels/channel.h"
#include "src/main/ztrace.h"

static char *ztrace_name = NULL;
static GTimer *timer = NULL;
static FILE *ztrace_log = NULL;
static GString *ztrace_buf = NULL;
static double ztrace_chrono = 0;

static int idx[] = {TrapRead, TrapWrite, TrapJail, TrapUnjail, TrapExit, TrapFork
#ifdef ZVM_SOCKETS
    , TrapSocket, TrapBind, TrapConnect, TrapAccept, TrapListen, TrapRecv,
    TrapRecvfrom, TrapRecvmsg, TrapSend, TrapSendto, TrapSendmsg, TrapGetsockopt,
    TrapSetsockopt, TrapSelect, TrapPoll, TrapGethostbyname, TrapGethostbyaddr,
    TrapClose
#endif
    };
static char *function[] =
    {"TrapRead", "TrapWrite", "TrapJail", "TrapUnjail", "TrapExit", "TrapFork",
#ifdef ZVM_SOCKETS
    "TrapSocket", "TrapBind", "TrapConnect", "TrapAccept", "TrapListen",
    "TrapRecv", "TrapRecvfrom", "TrapRecvmsg", "TrapSend", "TrapSendto",
    "TrapSendmsg", "TrapGetsockopt", "TrapSetsockopt", "TrapSelect", "TrapPoll",
    "TrapGethostbyname", "TrapGethostbyaddr", "TrapClose",
#endif
    "Invalid"};

static char *fmt[] = {
    "%d, %p, %d, %ld", /* TrapRead */
    "%d, %p, %d, %ld", /* TrapWrite */
    "%p, %d", /* TrapJail */
    "%p, %d", /* TrapUnjail */
    "", /* TrapExit */
    "" /* TrapFork */
#ifdef ZVM_SOCKETS
    ,
    "%d, %d, %d", /* socket */
    "%d, %p, %u", /* bind */
    "%d, %p, %u", /* connect */
    "%d, %p, %p", /* accept */
    "%d, %d", /* listen */
    "%d, %p, %ld, %d", /* recv */
    "%d, %p, %ld, %d, %p, %p", /* recvfrom */
    "%d, %p, %d", /* recvmsg */
    "%d, %p, %ld, %d", /* send */
    "%d, %p, %ld, %d, %p, %u", /* sendto */
    "%d, %p, %d", /* sendmsg */
    "%d, %d, %d, %p, %p", /* getsockopt */
    "%d, %d, %d, %p, %p", /* setsockopt */
    "%d, %p, %p, %p, %p", /* select */
    "%p, %u, %ul", /* poll */
    "%s", /* gethostbyname */
    "%p, %u, %d", /* gethostbyaddr */
    "%d", /* close */
#endif
};

void ZTraceCtor(const char *name)
{
  /* set ztrace file name */
  if(ztrace_name == NULL && name == NULL) return;
  if(ztrace_name == NULL)
  {
    ZLOGFAIL(!g_path_is_absolute(name), EFAULT,
        "ztrace path should be absolute: %s", name);
    ztrace_name = g_strdup(name);
  }

  /* open ztrace file */
  ztrace_log = fopen(ztrace_name, "a");
  ZLOGFAIL(ztrace_log == NULL, errno, "cannot open %s", ztrace_name);

  /* initialize ztrace buffer */
  ztrace_buf = g_string_sized_new(BIG_ENOUGH_STRING);
  g_string_append_printf(ztrace_buf, "[%d] %048o\n", getpid(), 0);

  /* set timer */
  ztrace_chrono = 0;
  timer = g_timer_new();
}

void ZTraceDtor(int mode)
{
  int result;

  if(timer == NULL || ztrace_log == NULL || ztrace_buf == NULL) return;

  /* drop buffer to log */
  if(mode != 0)
  {
    g_string_append_printf(ztrace_buf, "\n");
    result = fwrite(ztrace_buf->str, 1, ztrace_buf->len, ztrace_log);
    ZLOGIF(result != ztrace_buf->len, "only %d written to ztrace", result);
    fflush(ztrace_log);
    fclose(ztrace_log);
  }

  /* free resources */
  g_string_free(ztrace_buf, TRUE);
  g_timer_destroy(timer);
}

void ZTraceNameDtor()
{
  g_free(ztrace_name);
  ztrace_name = NULL;
}

void ZTrace(const char *msg)
{
  double timing;

  if(timer == NULL || ztrace_log == NULL || ztrace_buf == NULL) return;

  timing = g_timer_elapsed(timer, NULL);
  ztrace_chrono += timing;
  g_string_append_printf(ztrace_buf, "%.6f [%.6f]: %s\n", ztrace_chrono, timing, msg);
  g_timer_start(timer);
}

static int FunctionIdx(int id)
{
  int i;

  for(i = 0; i < ARRAY_SIZE(idx); ++i)
    if(idx[i] == id) return i;
  return ARRAY_SIZE(idx);
}

char *FunctionName(int id)
{
  return function[FunctionIdx(id)];
}

void SyscallZTrace(int id, int retcode, ...)
{
  char *msg;
  char *buf;
  va_list ap;

  /* skip if ztrace is not initialized */
  if(ztrace_name == NULL) return;

  va_start(ap, retcode);
  buf = g_strdup_vprintf(fmt[FunctionIdx(id)], ap);

#ifdef ZVM_SOCKETS
  msg = g_strdup_printf((id == TrapGethostbyname || id == TrapGethostbyaddr)
      ? "%s(%s) = 0x%x" : "%s(%s) = %d", FunctionName(id), buf, retcode);
#else
  msg = g_strdup_printf("%s(%s) = %d", FunctionName(id), buf, retcode);
#endif

  va_end(ap);
  ZTrace(msg);
  g_free(msg);
  g_free(buf);
}
