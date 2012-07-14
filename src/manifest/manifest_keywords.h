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
  /* look at "doc/channels.txt" */\
  "Channel",\
\
  /* generic custom attributes (look at "doc/environment.txt") */\
  /* replaces "custom attributes for the user" below */\
  "Environment",\
\
  /* will replace "report request keywords" below */\
  /*"Report",*/\
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
  "Report", /* report file descriptor (will be removed) */\
  "Nexe", /* nexe file name */\
  "NexeMax", /* maximum allowed nexe size */\
  "NexeEtag", /* reserved for "fast validation" */\
  "Timeout", /* maximum zerovm time to run */\
  "MemMax", /* size of memory available for nexe */\
  "SyscallsMax", /* syscalls allowed nexe to invoke */\
  "CommandLine", /* command line for nexe */\
  "TheEnd" /* used to check if this array is in sync with the enum */\
};

/* should be synchronized with string array above */
enum ManifestKeywords
{
  Channel,

  /* custom user attributes (environment) */
  Environment,

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
  TheEnd /* used to check if this enum is in sync with the string array */
};

#endif /* MANIFEST_KEYWORDS_H_ */
