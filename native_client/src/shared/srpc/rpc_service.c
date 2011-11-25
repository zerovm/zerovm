/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Service descriptions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/shared/srpc/nacl_srpc.h"
#include "native_client/src/shared/srpc/nacl_srpc_internal.h"

#ifndef SIZE_T_MAX
# define SIZE_T_MAX (~((size_t) 0))
#endif


/*
 * The struct used to describe methods within services.
 */
struct NaClSrpcMethodDesc {
  char const  *name;
  char const  *input_types;
  char const  *output_types;
  /**
   * function pointer used to process calls to the named method
   */
  NaClSrpcMethod handler;
};
typedef struct NaClSrpcMethodDesc NaClSrpcMethodDesc;

/*
 * Forward declarations for static "built-in" methods.
 */
static void ServiceDiscovery(NaClSrpcRpc* rpc,
                             NaClSrpcArg** in_args,
                             NaClSrpcArg** out_args,
                             NaClSrpcClosure* done);

/*
 * Get the next text element (name, input types, or output types).  These
 * elements are delimited by ':', '\n', or '\0'.
 */
static char* GetOneElement(const char* string) {
  size_t len = 0;
  char* buf;

  while (':' != string[len] && '\n' != string[len] && '\0' != string[len]) {
    len++;
  }
  buf = malloc(len + 1);
  if (NULL == buf) {
    return NULL;
  }
  strncpy(buf, string, len);
  buf[len] = '\0';
  return buf;
}

/* * Get the components of one method description.  Returns a pointer to
 * the delimiter if successful or NULL if not.
 */
static const char* ParseOneEntry(const char* entry_fmt,
                                 char** name,
                                 char** input_types,
                                 char** output_types) {
  char* namebuf = NULL;
  char* insbuf = NULL;
  char* outsbuf = NULL;

  *name = NULL;
  *input_types = NULL;
  *output_types = NULL;

  /* Don't try to parse NULL strings. */
  if (NULL == entry_fmt) {
    goto cleanup;
  }
  /* Get the name into a buffer, and ensure it is followed by ':'. */
  namebuf = GetOneElement(entry_fmt);
  if (NULL == namebuf) {
    goto cleanup;
  }
  entry_fmt += strlen(namebuf);
  if (':' != *entry_fmt) {
    goto cleanup;
  }
  entry_fmt++;
  /* Get the input types into a buffer, and ensure it is followed by ':'. */
  insbuf = GetOneElement(entry_fmt);
  if (NULL == insbuf) {
    goto cleanup;
  }
  entry_fmt += strlen(insbuf);
  if (':' != *entry_fmt) {
    goto cleanup;
  }
  entry_fmt++;
  /* Get the output types into a buffer. */
  outsbuf = GetOneElement(entry_fmt);
  if (NULL == outsbuf) {
    goto cleanup;
  }
  entry_fmt += strlen(outsbuf);
  /* Copy the result strings out. */
  *name = namebuf;
  *input_types = insbuf;
  *output_types = outsbuf;
  /* Return a pointer to the next character after the elements. */
  return entry_fmt;

 cleanup:
  free(insbuf);
  free(namebuf);
  return NULL;
}

/*
 * The method tables passed to construction do not contain "intrinsic" methods
 * such as service discovery and shutdown.  Build a complete table including
 * those from a given input.
 */
static NaClSrpcMethodDesc* BuildMethods(
    const struct NaClSrpcHandlerDesc* methods,
    uint32_t* method_count) {
  static const char* kSDDescString = "service_discovery::C";
  NaClSrpcMethodDesc* complete_methods;
  uint32_t i;

  /* Compute the number of methods to export. */
  *method_count = 0;
  while (NULL != methods[*method_count].entry_fmt)
    ++*method_count;
  /* Add one extra method for service discovery. */
  ++*method_count;
  /* Allocate the method descriptors. One extra for NULL termination. */
  complete_methods = (NaClSrpcMethodDesc*)
      malloc((*method_count + 1) * sizeof(*complete_methods));
  if (NULL == complete_methods) {
    return NULL;
  }
  /* Copy the methods passed in, adding service discovery as element zero. */
  ParseOneEntry(kSDDescString,
                (char**) &complete_methods[0].name,
                (char**) &complete_methods[0].input_types,
                (char**) &complete_methods[0].output_types);
  complete_methods[0].handler = ServiceDiscovery;
  for (i = 0; i < *method_count - 1; ++i) {
    ParseOneEntry(methods[i].entry_fmt,
                  (char**) &complete_methods[i + 1].name,
                  (char**) &complete_methods[i + 1].input_types,
                  (char**) &complete_methods[i + 1].output_types);
    complete_methods[i + 1].handler = methods[i].handler;
  }
  /* Add the NULL terminator */
  complete_methods[*method_count].name = NULL;
  complete_methods[*method_count].input_types = NULL;
  complete_methods[*method_count].output_types = NULL;
  complete_methods[*method_count].handler = NULL;
  /* Return the array */
  return complete_methods;
}

/*
 * Builds a service discovery string from an array of method descriptions.
 */
static char* BuildSDString(const NaClSrpcMethodDesc* methods,
                           uint32_t method_count,
                           nacl_abi_size_t *length) {
  uint32_t i;
  char* p;
  char* str;

  /* Find the total length of the method description strings.  */
  *length = 1;
  for (i = 0; i < method_count; ++i) {
    *length += nacl_abi_size_t_saturate(
                 strlen(methods[i].name) + 1 +
                 strlen(methods[i].input_types) + 1 +
                 strlen(methods[i].output_types) + 1);
  }
  /* Allocate the string. */
  str = (char*) malloc(*length + 1);
  if (NULL == str) {
    return NULL;
  }
  /* Concatenate the method description strings into the string. */
  p = str;
  for (i = 0; i < method_count; ++i) {
    size_t buf_limit = str + *length - p;
    /* TODO(robertm): explore use portability_io.h */
#if NACL_WINDOWS
    p += _snprintf(p, buf_limit, "%s:%s:%s\n",
                   methods[i].name,
                   methods[i].input_types,
                   methods[i].output_types);
#else
    p += snprintf(p, buf_limit, "%s:%s:%s\n",
                  methods[i].name,
                  methods[i].input_types,
                  methods[i].output_types);
#endif
  }
  *p = '\0';
  /* Return the resulting string. */
  return str;
}

static void FreeMethods(NaClSrpcMethodDesc* methods, uint32_t method_count) {
  uint32_t i;

  if (NULL == methods) {
    return;
  }
  for (i = 0; i < method_count; ++i) {
    if (NULL == methods[i].name) {
      /* We have reached the end of the portion set by ParseOneEntry calls. */
      break;
    }
    free((char*) methods[i].name);
    free((char*) methods[i].input_types);
    free((char*) methods[i].output_types);
  }
  free(methods);
}

/*
 * Create a service descriptor from an array of methods.
 */
int NaClSrpcServiceHandlerCtor(NaClSrpcService* service,
                               const NaClSrpcHandlerDesc* handler_desc) {
  char*       service_str;
  nacl_abi_size_t  str_length;
  NaClSrpcMethodDesc* methods = NULL;
  uint32_t method_count = 0;

  /* Initialize the struct, so that failures can be cleaned up properly. */
  service->service_string = NULL;
  service->service_string_length = 0;
  service->rpc_descr = NULL;
  service->rpc_count = 0;
  /* Add the service_discovery method to the table. */
  methods = BuildMethods(handler_desc, &method_count);
  if (NULL == methods) {
    goto cleanup;
  }
  service_str = BuildSDString(methods, method_count, &str_length);
  if (NULL == service_str) {
    goto cleanup;
  }
  service->service_string = service_str;
  service->service_string_length = str_length;
  service->rpc_descr = methods;
  service->rpc_count = method_count;
  return 1;
 cleanup:
  FreeMethods(methods, method_count);
  return 0;
}

int NaClSrpcServiceStringCtor(NaClSrpcService* service, const char* str) {
  NaClSrpcMethodDesc* methods = NULL;
  const char* p;
  uint32_t i;
  uint32_t rpc_count;
  size_t rpc_count_size_t;

  /* Initialize the struct, so that failures can be cleaned up properly. */
  service->service_string = NULL;
  service->service_string_length = 0;
  service->rpc_descr = NULL;
  service->rpc_count = 0;
  /* Count the number of rpc methods */
  rpc_count = 0;
  for (p = str; *p != '\0'; ) {
    char* next_p = strchr(p, '\n');
    if (NULL == next_p) {
      /* malformed input -- no remaining \n character was found */
      goto cleanup;
    }
    p = next_p + 1;
    ++rpc_count;
    if (0 == rpc_count) {
      /* uint32_t overflow detected. */
      goto cleanup;
    }
  }
  /*
   * The front end knows the next comparison is useless due to the range of
   * uint32_t.  And furthermore at least one version of gcc knows that a
   * cast doesn't really change that fact.  Hence, assign to a new variable
   * of size_t type.
   */
  rpc_count_size_t = (size_t) rpc_count;
  /* Allocate and clear the descriptor array */
  if (rpc_count_size_t >= SIZE_T_MAX / sizeof(*methods)) {
    goto cleanup;
  }
  methods = (NaClSrpcMethodDesc*) malloc(rpc_count_size_t * sizeof(*methods));
  if (NULL == methods) {
    goto cleanup;
  }
  memset(methods, 0, rpc_count_size_t * sizeof(*methods));
  /* Parse the list of method descriptions */
  p = str;
  for (i = 0; i < rpc_count; ++i) {
    const char* newline_loc;

    newline_loc = ParseOneEntry(p,
                                (char**) &methods[i].name,
                                (char**) &methods[i].input_types,
                                (char**) &methods[i].output_types);
    if (NULL == newline_loc || '\n' != *newline_loc) {
      goto cleanup;
    }
    p = newline_loc + 1;
    /* The handler is not set from service_discovery strings */
    methods[i].handler = NULL;
  }
  service->service_string = strdup(str);
  service->service_string_length = nacl_abi_size_t_saturate(strlen(str));
  service->rpc_descr = methods;
  service->rpc_count = rpc_count;
  return 1;

 cleanup:
  FreeMethods(methods, rpc_count);
  return 0;
}

/*
 * Destroy a service descriptor.
 */
void NaClSrpcServiceDtor(NaClSrpcService* service) {
  if (NULL == service) {
    return;
  }
  /* Free the method descriptors. */
  FreeMethods((NaClSrpcMethodDesc*) service->rpc_descr, service->rpc_count);
  /* Free the service discovery string. */
  free((char*) service->service_string);
}

void NaClSrpcServicePrint(const NaClSrpcService *service) {
  uint32_t i;

  if (NULL == service) {
    printf("NULL service\n");
    return;
  }
  printf("RPC %-20s %-10s %-10s\n", "Name", "Input args", "Output args");
  for (i = 0; i < service->rpc_count; ++i) {
    printf("%3u %-20s %-10s %-10s\n",
           (unsigned int) i,
           service->rpc_descr[i].name,
           service->rpc_descr[i].input_types,
           service->rpc_descr[i].output_types);
  }
}

uint32_t NaClSrpcServiceMethodCount(const NaClSrpcService *service) {
  if (NULL == service) {
    return 0;
  }
  return service->rpc_count;
}

static int SignatureMatches(NaClSrpcMethodDesc* method_desc,
                            char const* signature) {
  struct {
    char const* field;
    char terminator;
  } matcher[3];
  size_t ix;

  matcher[0].field = method_desc->name;
  matcher[0].terminator = ':';
  matcher[1].field = method_desc->input_types;
  matcher[1].terminator = ':';
  matcher[2].field = method_desc->output_types;
  matcher[2].terminator = '\0';

  for (ix = 0; ix < sizeof(matcher) / sizeof(matcher[0]); ++ix) {
    size_t length = strlen(matcher[ix].field);
    if (strncmp(matcher[ix].field, signature, length) ||
        matcher[ix].terminator != signature[length]) {
      return 0;
    }
    signature += length + 1;
  }
  return 1;
}

uint32_t NaClSrpcServiceMethodIndex(const NaClSrpcService* service,
                                    char const* signature) {
  uint32_t i;

  if (NULL == service) {
    return kNaClSrpcInvalidMethodIndex;
  }
  for (i = 0; i < service->rpc_count;  ++i) {
    if (SignatureMatches(&(service->rpc_descr[i]), signature)) {
      return i;
    }
  }
  return kNaClSrpcInvalidMethodIndex;
}

int NaClSrpcServiceMethodNameAndTypes(const NaClSrpcService* service,
                                      uint32_t rpc_number,
                                      const char** name,
                                      const char** input_types,
                                      const char** output_types) {
  if (rpc_number >= service->rpc_count) {
    /* This ensures that the method is in the user-defined set. */
    return 0;
  } else {
    *name = service->rpc_descr[rpc_number].name;
    *input_types = service->rpc_descr[rpc_number].input_types;
    *output_types = service->rpc_descr[rpc_number].output_types;
  }
  return 1;
}

NaClSrpcMethod NaClSrpcServiceMethod(const NaClSrpcService* service,
                                     uint32_t rpc_number) {
  if (NULL == service || rpc_number >= service->rpc_count) {
    return NULL;
  } else {
    return service->rpc_descr[rpc_number].handler;
  }
}

static void ServiceDiscovery(NaClSrpcRpc* rpc,
                             NaClSrpcArg** in_args,
                             NaClSrpcArg** out_args,
                             NaClSrpcClosure* done) {
  UNREFERENCED_PARAMETER(in_args);
  rpc->result = NACL_SRPC_RESULT_APP_ERROR;
  if (NULL == rpc->channel || NULL == rpc->channel->server) {
    done->Run(done);
    return;
  }
  if (out_args[0]->u.count >= rpc->channel->server->service_string_length) {
    strncpy(out_args[0]->arrays.carr,
            rpc->channel->server->service_string,
            rpc->channel->server->service_string_length);
    /* Set the length of the string actually returned. */
    out_args[0]->u.count = rpc->channel->server->service_string_length;
    rpc->result = NACL_SRPC_RESULT_OK;
  }
  done->Run(done);
}
