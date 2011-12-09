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
  Input,
  InputMax, /* file length limit */
  InputMaxGet,
  InputMaxGetCnt,
  InputMaxPut, /* n/a */
  InputMaxPutCnt, /* n/a */
  InputMode,
  Output,
  OutputMax, /* file length limit */
  OutputMaxGet,
  OutputMaxGetCnt,
  OutputMaxPut,
  OutputMaxPutCnt,
  OutputMode,
  UserLog,
  UserLogMax, /* file length limit */
  UserMaxLogGet, /* n/a */
  UserMaxLogGetCnt, /* n/a */
  UserMaxLogPut, /* n/a */
  UserMaxLogPutCnt, /* n/a */
  UserLogMode, /* n/a */
  NetInput,
  NetInputMax, /* file length limit */
  NetInputMaxGet,
  NetInputMaxGetCnt,
  NetInputMaxPut,
  NetInputMaxPutCnt,
  NetInputMode,
  NetOutput,
  NetOutputMax, /* file length limit */
  NetOutputMaxGet,
  NetOutputMaxGetCnt,
  NetOutputMaxPut,
  NetOutputMaxPutCnt,
  NetOutputMode,
};

/* user side keywords */
enum UserKeys {
  ContentType,
  TimeStamp,
  XObjectMetaTag, /* only duplicate keyword */
  UserETag
};

/* report request keywords */
enum ReportKeys {
  ReportRetCode,
  ReportEtag,
  ReportUserRetCode,
  ReportContentType,
  ReportXObjectMetaTag
};

/* zerovm control keywords */
enum Control {
  Version,
  ZeroVM,
  Log,
  Report,
  Nexe,
  NexeMax,
  NexeEtag,
  Timeout,
  KillTimeout,
  MemMax,
  CPUMax,
  SyscallsMax,
  SetupCallsMax,
  /* new */
  Blob, /* blob library if it will retain */
  CommandLine /* command line for nexe */
};

#endif /* MANIFEST_KEYWORDS_H_ */
