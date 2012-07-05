/*
 * manifest_keywords.h is not supposed to be part of zerovm source code
 * content of this file should be moved to documentation
 *
 * update: all keywords now put into the string array indexed with enumeration
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */
#ifndef MANIFEST_KEYWORDS_H_
#define MANIFEST_KEYWORDS_H_

/*
 * this macro substitution can be used to create array of keywords
 * should be synchronized with enum ManifestKeywords
 */
#define MANIFEST_KEYWORDS {\
  /* i/o channels keywords */\
  "Input", /* name of the input channel/file */\
  "InputMaxGet", /* bytes count allowed to get */\
  "InputMaxGetCnt", /* how many times allowed to invoke "get" syscall. n/a for mounted resiources */\
  "InputMaxPut", /* n/a */\
  "InputMaxPutCnt", /* n/a */\
  "InputMode", /* 0 - premounted channel, 1 - preloaded, 2 - preallocated from network */\
  "Output", /* name of the output channel/file */\
  "OutputMaxGet", /* bytes count allowed to get */\
  "OutputMaxGetCnt", /* how many times allowed to invoke "get" syscall. n/a for mounted resiources */\
  "OutputMaxPut", /* bytes count allowed to put */\
  "OutputMaxPutCnt", /* how many times allowed to invoke "put" syscall. n/a for mounted resiources */\
  "OutputMode", /* 0 - premounted channel, 1 - preloaded, 2 - preallocated from network */\
  "UserLog", /* user log file name. gets/puts/e.t.c. are unlimited */\
  "UserLogMaxGet", /* n/a */\
  "UserLogMaxGetCnt", /* n/a */\
  "UserLogMaxPut", /* n/a */\
  "UserLogMaxPutCnt", /* n/a */\
  "UserLogMode", /* n/a */\
  "NetInput",\
  "NetInputMaxGet",\
  "NetInputMaxGetCnt",\
  "NetInputMaxPut",\
  "NetInputMaxPutCnt",\
  "NetInputMode",\
  "NetOutput",\
  "NetOutputMaxGet",\
  "NetOutputMaxGetCnt",\
  "NetOutputMaxPut",\
  "NetOutputMaxPutCnt",\
  "NetOutputMode",\
\
  /* user side keywords */\
  "ContentType",\
  "TimeStamp", /* time/seed */\
  "XObjectMetaTag", /* custom attributes */\
  "UserETag", /* checksum of the user output */\
\
/*\
 * report request keywords\
 * note: values should exist to make valid manifest record\
 *       but they will be ignored. recommended way is: ReportEtag = :)\
 *       NOT recommended, misleading way: ReportEtag = yes\
 */\
  "ReportRetCode", /* exit code of zerovm */\
  "ReportEtag", /* checksum of the user output */\
  "ReportUserRetCode", /* exit code of the user program */\
  "ReportContentType",\
  "ReportXObjectMetaTag", /* custom attributes set by user */\
\
  /* zerovm control keywords */\
  "Version", /* zerovm version */\
  "Log", /* zerovm log file name */\
  "Report", /* report file descriptor */\
  "Nexe", /* nexe file name */\
  "NexeMax", /* maximum allowed nexe size */\
  "NexeEtag", /* reserved for "fast validation" */\
  "Timeout", /* maximum zerovm time to run */\
  "MemMax", /* size of memory available for nexe */\
  "SyscallsMax", /* syscalls allowed nexe to invoke */\
  "CommandLine", /* command line for nexe */\
  "Networking", /* todo(d'b): remove it */\
  "TheEnd" /* used to check if this array is in sync with the enum */\
};

/* should be synchronized with string array above */
enum ManifestKeywords
{
  /*
   * i/o channels keywords
   */
  Input, /* name of the input channel/file */
  InputMaxGet, /* bytes count allowed to get */
  InputMaxGetCnt, /* how many times allowed to invoke "get" syscall. n/a for mounted resiources */
  InputMaxPut, /* n/a */
  InputMaxPutCnt, /* n/a */
  InputMode, /* 0 - premounted channel, 1 - preloaded, 2 - preallocated from network */
  Output, /* name of the output channel/file */
  OutputMaxGet, /* bytes count allowed to get */
  OutputMaxGetCnt, /* how many times allowed to invoke "get" syscall. n/a for mounted resiources */
  OutputMaxPut, /* bytes count allowed to put */
  OutputMaxPutCnt, /* how many times allowed to invoke "put" syscall. n/a for mounted resiources */
  OutputMode, /* 0 - premounted channel, 1 - preloaded, 2 - preallocated from network */
  UserLog, /* user log file name. gets/puts/e.t.c. are unlimited */
  UserLogMaxGet, /* n/a */
  UserLogMaxGetCnt, /* n/a */
  UserLogMaxPut, /* n/a */
  UserLogMaxPutCnt, /* n/a */
  UserLogMaxMode, /* n/a */
  NetInput,
  NetInputMaxGet,
  NetInputMaxGetCnt,
  NetInputMaxPut,
  NetInputMaxPutCnt,
  NetInputMode,
  NetOutput,
  NetOutputMaxGet,
  NetOutputMaxGetCnt,
  NetOutputMaxPut,
  NetOutputMaxPutCnt,
  NetOutputMode,

  /*
   * user side keywords
   */
  ContentType,
  TimeStamp, /* time/seed */
  XObjectMetaTag, /* custom attributes */
  UserETag, /* checksum of the user output */

  /*
   * report request keywords
   * note: values should exist to make valid manifest record
   *       but they will be ignored. recommended way is: ReportEtag = :)
   *       NOT recommended, misleading way: ReportEtag = yes
   */
  ReportRetCode, /* exit code of zerovm */
  ReportEtag, /* checksum of the user output */
  ReportUserRetCode, /* exit code of the user program */
  ReportContentType,
  ReportXObjectMetaTag, /* custom attributes set by user */

  /*
   *  zerovm control keywords
   */
  Version, /* zerovm version */
  Log, /* zerovm log file name */
  Report, /* report file descriptor */
  Nexe, /* nexe file name */
  NexeMax, /* maximum allowed nexe size */
  NexeEtag, /* reserved for "fast validation" */
  Timeout, /* maximum zerovm time to run */
  MemMax, /* size of memory available for nexe */
  SyscallsMax, /* syscalls allowed nexe to invoke */
  CommandLine, /* command line for nexe */
  Networking, /* todo(d'b): added temporary, remove it */
  TheEnd /* used to check if this enum is in sync with the string array */
};

#endif /* MANIFEST_KEYWORDS_H_ */
