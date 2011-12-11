/*
 * manifest_keywords.h is not supposed to be part of zerovm source code
 * content of this file should be moved to documentation
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */

#ifndef MANIFEST_KEYWORDS_H_
#define MANIFEST_KEYWORDS_H_

/* i/o channels keywords */
enum IOKeys {
  Input, /* name of the input channel/file */
  InputMax, /* channel/file length limit */
  InputMaxGet, /* bytes count allowed to get */
  InputMaxGetCnt, /* how many times allowed to invoke "get" syscall. n/a for mounted resiources */
  InputMaxPut, /* n/a */
  InputMaxPutCnt, /* n/a */
  InputMode, /* 0 - premounted channel, 1 - preloaded, 2 - preallocated from network */
  Output, /* name of the output channel/file */
  OutputMax, /* channel/file length limit */
  OutputMaxGet, /* bytes count allowed to get */
  OutputMaxGetCnt, /* how many times allowed to invoke "get" syscall. n/a for mounted resiources */
  OutputMaxPut, /* bytes count allowed to put */
  OutputMaxPutCnt, /* how many times allowed to invoke "put" syscall. n/a for mounted resiources */
  OutputMode, /* 0 - premounted channel, 1 - preloaded, 2 - preallocated from network */
  UserLog, /* user log file name. gets/puts/e.t.c. are unlimited */
  UserLogMax, /* file length limit */
  UserMaxLogGet, /* n/a */
  UserMaxLogGetCnt, /* n/a */
  UserMaxLogPut, /* n/a */
  UserMaxLogPutCnt, /* n/a */
  UserLogMode, /* n/a */
  NetInput,
  NetInputMax, /* limit for receive */
  NetInputMaxGet,
  NetInputMaxGetCnt,
  NetInputMaxPut,
  NetInputMaxPutCnt,
  NetInputMode,
  NetOutput,
  NetOutputMax, /* limit for send */
  NetOutputMaxGet,
  NetOutputMaxGetCnt,
  NetOutputMaxPut,
  NetOutputMaxPutCnt,
  NetOutputMode,
};

/* user side keywords */
enum UserKeys {
  ContentType,
  TimeStamp, /* time/seed */
  XObjectMetaTag, /* custom attributes */
  UserETag /* checksum of the user output */
};

/* report request keywords */
enum ReportKeys {
  ReportRetCode, /* exit code of zerovm */
  ReportEtag, /* checksum of the user output */
  ReportUserRetCode, /* exit code of the user program */
  ReportContentType,
  ReportXObjectMetaTag /* custom attributes set by user */
};

/* zerovm control keywords */
enum Control {
  Version, /* zerovm version */
  ZeroVM,
  Log, /* zerovm log file name */
  Report, /* report file name */
  Nexe, /* nexe file name */
  NexeMax, /* maximum allowed nexe size */
  NexeEtag, /* reserved for "fast validation" */
  Timeout, /* maximum zerovm time to run */
  KillTimeout, /* zerovm time to live */
  MemMax, /* size of memory available for nexe */
  CPUMax, /* cpu time allotted to nexe */
  SyscallsMax, /* syscalls allowed nexe to invoke */
  SetupCallsMax, /* setup calls allowed nexe to invoke */
  Blob, /* blob library if it will retain */
  CommandLine /* command line for nexe */
};

#endif /* MANIFEST_KEYWORDS_H_ */
