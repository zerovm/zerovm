/*
 * manifest_keywords.h is not supposed to be part of zerovm source code
 * content of this file should be moved to documentation
 *
 * update: all keywords now put into the string array indexed with enumeration
 * todo(d'b): rewrite it using x-macro
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
\
  /* zerovm control keywords */\
  "Version", /* zerovm version */\
  "Nexe", /* nexe file name */\
  "NexeMax", /* maximum allowed nexe size */\
  "NexeEtag", /* reserved for "fast validation" */\
  "Timeout", /* maximum nexe time to run */\
  "MemMax", /* size of memory available for nexe */\
  "SyscallsMax", /* syscalls allowed nexe to invoke */\
  "CommandLine", /* command line for nexe */\
  "NodeName", /* node name. available for user as argv[0] */\
  "TheEnd" /* used to check if this array is in sync with the enum */\
};

/* should be synchronized with string array above */
enum ManifestKeywords
{
  Channel,

  /* custom user attributes (environment) */
  Environment,

  /*
   *  zerovm control keywords
   */
  Version, /* zerovm version */
  Nexe, /* nexe file name */
  NexeMax, /* maximum allowed nexe size */
  NexeEtag, /* reserved for "fast validation" */
  Timeout, /* maximum nexe time to run */
  MemMax, /* size of memory available for nexe */
  SyscallsMax, /* syscalls allowed nexe to invoke */
  CommandLine, /* command line for nexe */
  NodeName, /* node name. available for user as argv[0] */
  TheEnd /* used to check if this enum is in sync with the string array */
};

#endif /* MANIFEST_KEYWORDS_H_ */
