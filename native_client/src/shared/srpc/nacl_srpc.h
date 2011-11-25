/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_SHARED_SRPC_NACL_SRPC_H_
#define NATIVE_CLIENT_SRC_SHARED_SRPC_NACL_SRPC_H_

/**
 * @file
 * Defines the API in the
 * <a href="group___s_r_p_c.html">Simple RPC (SRPC) library</a>
 *
 * @addtogroup SRPC
 * @{
 */

#include <stdarg.h>
#include <stdio.h>

/*
 * TODO(sehr) break this file into separate files for sdk and service_runtime
 * inclusion.
 */
#ifdef __native_client__
#  include <machine/_types.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS
/*
 * Avoid emacs' penchant for auto-indenting extern "C" blocks.
 */
#  ifdef __cplusplus
#    define EXTERN_C_BEGIN extern "C" {
#    define EXTERN_C_END   }
#  else
#    define EXTERN_C_BEGIN
#    define EXTERN_C_END
#  endif  /* __cplusplus */
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * Contains a file descriptor for use as an IMC channel.
 */
typedef int NaClSrpcImcDescType;
#define kNaClSrpcInvalidImcDesc -1
#else
#  include "native_client/src/include/portability.h"
#  include "native_client/src/trusted/service_runtime/include/machine/_types.h"
/*
 * In trusted code we use a NaClDesc to describe the IMC channel.
 * It is this difference between trusted and untrusted code that motivated
 * creating a type name.
 */
typedef struct NaClDesc* NaClSrpcImcDescType;
#define kNaClSrpcInvalidImcDesc NULL
#endif

EXTERN_C_BEGIN


/**
 * Result codes to be returned from Simple RPC services.
 */
typedef enum NaClSrpcResultCodes {
  /** Invocation was successful. */
  NACL_SRPC_RESULT_OK = 256,
  /** Invocation was successful, exit the server dispatch loop.
   *  When returned from an RPC method, this causes the server to return
   *  NACL_SRPC_RESULT_OK and then exit the receive-dispatch-reply loop.
   */
  NACL_SRPC_RESULT_BREAK,
  /** Some or all of a message was not received. */
  NACL_SRPC_RESULT_MESSAGE_TRUNCATED,
  /** The SRPC runtime system ran out of memory. */
  NACL_SRPC_RESULT_NO_MEMORY,
  /** The SRPC message layer detected a protocol version difference. */
  NACL_SRPC_RESULT_PROTOCOL_MISMATCH,
  /** The RPC number was not valid for any method. */
  NACL_SRPC_RESULT_BAD_RPC_NUMBER,
  /** An unknown type was passed to or returned from an RPC. */
  NACL_SRPC_RESULT_BAD_ARG_TYPE,
  /** Too few arguments were passed to or returned from an RPC. */
  NACL_SRPC_RESULT_TOO_FEW_ARGS,
  /** Too many arguments were passed to or returned from an RPC. */
  NACL_SRPC_RESULT_TOO_MANY_ARGS,
  /** One or more input argument types did not match the expected types. */
  NACL_SRPC_RESULT_IN_ARG_TYPE_MISMATCH,
  /** One or more output argument types did not match the expected types. */
  NACL_SRPC_RESULT_OUT_ARG_TYPE_MISMATCH,
  /** An unknown SRPC internal error occurred. */
  NACL_SRPC_RESULT_INTERNAL,
  /** The application signalled a generic error. */
  NACL_SRPC_RESULT_APP_ERROR
} NaClSrpcError;

/**
 *  Returns a descriptive string for the specified NaClSrpcError value.
 *  @param error_val A NaClSrpcError to be translated to a string.
 *  @return If error_val is valid, returns a string.  Otherwise returns the
 *  string "Unrecognized NaClSrpcError value"
 */
extern const char* NaClSrpcErrorString(NaClSrpcError error_val);

/**
 * Type tag values for NaClSrpcArg unions.
 */
enum NaClSrpcArgType {
  NACL_SRPC_ARG_TYPE_INVALID = 'X',  /**< invalid type */
  NACL_SRPC_ARG_TYPE_BOOL = 'b',  /**< scalar bool */
  NACL_SRPC_ARG_TYPE_CHAR_ARRAY = 'C',  /**< array of char */
  NACL_SRPC_ARG_TYPE_DOUBLE = 'd',  /**< scalar double */
  NACL_SRPC_ARG_TYPE_DOUBLE_ARRAY = 'D',  /**< array of double */
  NACL_SRPC_ARG_TYPE_HANDLE = 'h',  /**< a descriptor (handle) */
  NACL_SRPC_ARG_TYPE_INT = 'i',  /**< scalar int32_t */
  NACL_SRPC_ARG_TYPE_INT_ARRAY = 'I',  /**< array of int32_t */
  NACL_SRPC_ARG_TYPE_LONG = 'l',  /**< scalar int64_t */
  NACL_SRPC_ARG_TYPE_LONG_ARRAY = 'L',  /**< array of int64_t */
  NACL_SRPC_ARG_TYPE_STRING = 's',  /**< NUL-terminated string */
  NACL_SRPC_ARG_TYPE_OBJECT = 'o',  /**< scriptable object */
  NACL_SRPC_ARG_TYPE_VARIANT_ARRAY = 'A'  /**< array of NaClSrpcArg structs */
};

#ifdef __cplusplus
namespace nacl_srpc {
class ScriptableHandleBase;
}
#endif

#define NACL_SRPC_ARG_SERIALIZED_FIELDS                                    \
  /**                                                                      \
   * Determines which type this argument contains.  Its value determines   \
   * which element of union <code>u</code> may be validly referred to.     \
   */                                                                      \
  enum NaClSrpcArgType  tag;                                               \
  /**                                                                      \
   * Padding (unused) to ensure 8-byte alignment of the union u.           \
   */                                                                      \
  uint32_t              reserved_pad;                                      \
  /*                                                                       \
   * For efficiency, doubles should be 8-byte aligned so that              \
   * loading them would require a single bus cycle, and arrays of          \
   * NaClSrpcArgs will have to be 8-byte aligned as well, so it's          \
   * either a 4 byte padding between the tag and the union or              \
   * (implicitly) at the end.  gcc does not (by default) enforce           \
   * 8-byte alignment but visual studio does, so we explicitly pad         \
   * so that both compilers will use the same memory layout, even if       \
   * the gcc -malign-double flag were omitted.                             \
   */                                                                      \
  /**                                                                      \
   * A union containing the value of the argument.  The element containing \
   * the valid data is determined by <code>tag</code>.                     \
   */                                                                      \
  union {                                                                  \
    /** A boolean scalar value */                                          \
    int                 bval;                                              \
    /** An array size (count) value */                                     \
    nacl_abi_size_t     count;                                             \
    /** A double-precision floating point scalar value */                  \
    double              dval;                                              \
    /** A handle used to pass descriptors */                               \
    NaClSrpcImcDescType hval;                                              \
    /** An integer scalar value */                                         \
    int32_t             ival;                                              \
    /** A int64_t scalar value */                                          \
    int64_t             lval;                                              \
    /** An object value that can be exported to the browser as is */       \
  } u

/**
 *  Used to convey parameters to and from RPC invocations.  It is a variant
 *  type, with the <code>tag</code> telling whether to treat the parameter as
 *  a bool or an array of characters, etc.
 */
struct NaClSrpcArg {
  /* Data that is serialized for a value or template, regardless of type. */
  NACL_SRPC_ARG_SERIALIZED_FIELDS;
  /* Data that used to represent an array value, but is not serialized. */
  union {
    char                *carr;
    double              *darr;
    int32_t             *iarr;
    int64_t             *larr;
    /** This field is ordinarily used with predeclared methods, and is never
     *  used directly for communicating with nexes.  It usually points to an
     *  object scriptable by the browser, i.e.  NPAPI's NPObject or PPAPI's
     *  ScriptableObject.  Declaring it as void* to avoid including browser
     *  specific types here.
     *  TODO(polina): where can one assert for this?
     *  Also, as these pointers are overlaid, oval is used in array object
     *  serialization to avoid repeated type checks.
     */
    void                *oval;
    char                *str;
    struct NaClSrpcArg  *varr;
  } arrays;
};

/**
 * A typedef for struct NaClSrpcArg for use in C.
 */
typedef struct NaClSrpcArg NaClSrpcArg;

/**
 * A constructor that ensures argument memory is properly initialized.
 */
void NaClSrpcArgCtor(NaClSrpcArg* arg);

#define NACL_SRPC_RPC_SERIALIZED_FIELDS        \
  uint32_t                  protocol_version;  \
  uint32_t                  request_id;        \
  uint32_t                  is_request;        \
  uint32_t                  rpc_number;        \
  NaClSrpcError             result;            \
  uint32_t                  value_len;         \
  uint32_t                  template_len

/**
 * Remote procedure call state structure.
 */
struct NaClSrpcRpc {
  /* State that is serialized. */
  NACL_SRPC_RPC_SERIALIZED_FIELDS;
  /* State maintained for transmission/reception, but not serialized */
  struct NaClSrpcChannel*   channel;
  const char*               ret_types;
  NaClSrpcArg**             rets;
  uint8_t                   ret_send_succeeded;
  uint8_t                   dispatch_loop_should_continue;
};

/**
 * A typedef for struct NaClSrpcRpc for use in C.
 */
typedef struct NaClSrpcRpc NaClSrpcRpc;

/* TODO(gregoryd) - duplicate string? */
/**
 * A utility macro to set a string-typed argument.
 */
#define STRINGZ_TO_SRPCARG(val, arg) do { \
    (arg).tag = NACL_SRPC_ARG_TYPE_STRING; \
    (arg).u.sval.str = (val); } while (0)

/**
 * The maximum number of arguments per SRPC routine.
 */
#define NACL_SRPC_MAX_ARGS                     128

/**
 * The maximum number of characters returnable from service discovery.
 */
#define NACL_SRPC_MAX_SERVICE_DISCOVERY_CHARS  4000

/** A pointer to an SRPC channel data structure. */
typedef struct NaClSrpcChannel *NaClSrpcChannelPtr;

/**
 * The closure used to send responses from a method.
 */
struct NaClSrpcClosure {
  void (*Run)(struct NaClSrpcClosure* self);
};

#ifdef __cplusplus
/**
 * An RAII helper class for using NaClSrpcClosures in C++.
 */
class NaClSrpcClosureRunner {
 public:
  explicit NaClSrpcClosureRunner(NaClSrpcClosure* closure) :
    closure_(closure) { }
  ~NaClSrpcClosureRunner() { closure_->Run(closure_); }

 private:
  NaClSrpcClosure* closure_;
};
#endif

/**
 * A typedef for struct NaClSrpcClosure for use in C.
 */
typedef struct NaClSrpcClosure NaClSrpcClosure;

/**
 * Methods used to implement SRPC services have this type signature.
 */
typedef void (*NaClSrpcMethod)(NaClSrpcRpc* rpc,
                               NaClSrpcArg* args[],
                               NaClSrpcArg* rets[],
                               NaClSrpcClosure* done);

/**
 * Binaries that implement SRPC methods (servers) describe their services
 * using these structures.
 */
struct NaClSrpcHandlerDesc {
  /**
   * a string containing "name:input_types:output_types"
   */
  char const *entry_fmt;
  /**
   * function pointer used to process calls to the named method
   */
  NaClSrpcMethod handler;
};

/**
 * A typedef for struct NaClSrpcHandlerDesc for use in C.
 */
typedef struct NaClSrpcHandlerDesc NaClSrpcHandlerDesc;

/**
 * A private structure type used to describe methods within NaClSrpcService.
 */
struct NaClSrpcMethodDesc;

/**
 * A description of the services available on a channel.
 */
struct NaClSrpcService {
  /**
   * A pointer to an array of RPC service descriptors used to type check and
   * dispatch RPC requests.
   */
  struct NaClSrpcMethodDesc*  rpc_descr;
  /** The number of elements in the <code>rpc_descr</code> array. */
  uint32_t                    rpc_count;
  /**
   * A zero terminated string containing name:ins:outs triples used to respond
   * to service_discovery RPCs.
   */
  const char*                 service_string;
  /** The length of <code>service_string</code> in bytes */
  nacl_abi_size_t             service_string_length;
};

/**
 *  A typedef for struct NaClSrpcService for use in C.
 */
typedef struct NaClSrpcService NaClSrpcService;

/**
 *  Constructs an SRPC service object from an array of handlers.
 *  @param service The service to be constructed.
 *  @param imc_desc handler_desc The handlers for each of the methods.
 *  @return On success, 1; on failure, 0.
 */
int NaClSrpcServiceHandlerCtor(NaClSrpcService* service,
                               const NaClSrpcHandlerDesc* handler_desc);

/**
 *  Constructs an SRPC service object from a service discovery string.
 *  @param service The service to be constructed.
 *  @param imc_desc handler_desc The string returned from a service discovery
 *  call.
 *  @return On success, 1; on failure, 0.
 */
int NaClSrpcServiceStringCtor(NaClSrpcService* service,
                              const char* service_discovery_string);

/**
 *  Destroys an SRPC service object.
 *  @param service The service to be destroyed.
 */
void NaClSrpcServiceDtor(NaClSrpcService* service);

/**
 *  Prints the methods available from the specified service to stdout.
 *  @param service A service.
 */
void NaClSrpcServicePrint(const NaClSrpcService *service);

/**
 *  Obtains the count of methods exported by the service.
 *  @param service The service to be examined.
 *  @return The number of methods exported by the service.
 */
uint32_t NaClSrpcServiceMethodCount(const NaClSrpcService *service);


#define kNaClSrpcInvalidMethodIndex ((uint32_t) ~0)
/**
 *  Obtains the index of the specified RPC name.
 *  @param service The service to be searched.
 *  @param signature The exported signature of the method.
 *  @return A non-negative index if the name was found in the channel's set of
 *  methods.  If the name was not found, it returns kNaClSrpcInvalidMethodIndex.
 */
uint32_t NaClSrpcServiceMethodIndex(const NaClSrpcService *service,
                                    char const *signature);

/**
 *  Obtains the name, input types, and output types of the specified RPC
 *  number.
 *  @param service The service to be searched.
 *  @param rpc_number The number of the rpc to be looked up.
 *  @param name The exported name of the method.
 *  @param input_types The types of the inputs of the method.
 *  @param output_types The types of the outputs of the method.
 *  @return On success, 1; on failure, 0.
 */
extern int NaClSrpcServiceMethodNameAndTypes(const NaClSrpcService* service,
                                             uint32_t rpc_number,
                                             const char** name,
                                             const char** input_types,
                                             const char** output_types);

/**
 *  Obtains the function pointer used to handle invocations of a given
 *  method number.
 *  @param service The service to be searched.
 *  @param rpc_number The number of the rpc to be looked up.
 *  @return On success, a pointer to the handler; on failure, NULL.
 */
extern NaClSrpcMethod NaClSrpcServiceMethod(const NaClSrpcService* service,
                                            uint32_t rpc_number);

/**
 * The encapsulation of all the data necessary for an RPC connection,
 * either client or server.
 */
struct NaClSrpcChannel {
  /** A pointer to the message channel used to send and receive RPCs */
  struct NaClSrpcMessageChannel *message_channel;
  /** The id of the next rpc request message sent over this channel */
  uint32_t                      next_outgoing_request_id;
  /**
   * The services implemented by this server.
   */
  NaClSrpcService               *server;
  /**
   * The services available to this client.
   */
  NaClSrpcService               *client;
  /**
   * A pointer to channel-specific data.  This allows RPC method
   * implementations to be used across multiple services while still
   * maintaining reentrancy
   */
  void                          *server_instance_data;
};

/**
 *  A typedef for struct NaClSrpcChannel for use in C.
 */
typedef struct NaClSrpcChannel NaClSrpcChannel;

/**
 *  Initialize an SRPC channel so that its pointers are NULL.  This makes it
 *  safe to call a Dtor on the channel, even if the Ctor has not been called.
 */
void NaClSrpcChannelInitialize(NaClSrpcChannel* channel);

/**
 *  Constructs an SRPC client object communicating over an IMC descriptor.
 *  Clients issue RPC requests and receive responses.
 *  @param channel The channel descriptor to be constructed.
 *  @param imc_desc The NaClSrpcImcDescType describing the IMC channel to
 *  communicate over.
 *  @return On success, 1; on failure, 0.
 */
int NaClSrpcClientCtor(NaClSrpcChannel *channel, NaClSrpcImcDescType imc_desc);

/**
 *  Constructs an SRPC server object communicating over an IMC descriptor.
 *  Servers wait for RPC requests, dispatch them to implementations, and return
 *  responses.
 *  @param channel The channel descriptor to be constructed.
 *  @param imc_desc The NaClSrpcImcDescType describing the IMC channel to
 *  communicate over.
 *  @param service A NaClSrpcService structure describing the service
 *  handled by this server.
 *  @param instance_data A value to be stored on the channel descriptor
 *  for conveying data specific to this particular server instance.
 *  @return On success, 1; on failure, 0.
 */
int NaClSrpcServerCtor(NaClSrpcChannel           *channel,
                       NaClSrpcImcDescType       imc_desc,
                       NaClSrpcService           *service,
                       void*                     instance_data);

/**
 *  Destroys the specified client or server channel.
 *  @param channel The channel to be destroyed.
 */
void NaClSrpcDtor(NaClSrpcChannel *channel);

/**
 *  Runs an SRPC receive-dispatch-respond loop on the specified
 *  NaClSrpcImcDescType object.
 *  @param imc_socket_desc A NaClSrpcImcDescType object that RPCs will
 *  communicate over.
 *  @param methods An array of NaClSrpcHandlerDesc structures
 *  describing the set of services handled by this server.
 *  @param instance_data A value to be stored on the channel
 *  descriptor for conveying data specific to this particular server
 *  instance.
 *  @return On success, 1; on failure, 0.
 */
int NaClSrpcServerLoop(NaClSrpcImcDescType              imc_socket_desc,
                       const struct NaClSrpcHandlerDesc methods[],
                       void                             *instance_data);

/**
 *  Initializes the SRPC module.
 *  @return Returns one on success, zero otherwise.
 */
int NaClSrpcModuleInit();

/**
 *  Shuts down the SRPC module.
 */
void NaClSrpcModuleFini();

/**
 *  @serverSrpc Waits for a connection from a client.  When a client
 *  connects, the server starts responding to RPC requests.
 *  @param methods The array of methods exported.
 *  @return Returns one on success, zero otherwise.
 */
int NaClSrpcAcceptClientConnection(const struct NaClSrpcHandlerDesc *methods);

/**
 *  @serverSrpc Waits for a connection from a client.  When a client
 *  connects, spawns a new thread with a server responding to RPC requests.
 *  @param methods The array of methods exported.
 *  @return Returns one on success, zero otherwise.
 */
int NaClSrpcAcceptClientOnThread(const struct NaClSrpcHandlerDesc *methods);

/**
 *  @clientSrpc Invokes a specified RPC on the given channel.  Parameters
 *  are passed by stdarg conventions and determined from the types specified
 *  in the method tables.
 *  @param channel The channel descriptor to use to invoke the RPC.
 *  @param rpc_name The name of the RPC to be invoked.
 *  @return A NaClSrpcResultCodes indicating success (NACL_SRPC_RESULT_OK)
 *  or failure.
 *  @see NaClSrpcResultCodes
 */
extern NaClSrpcError NaClSrpcInvokeBySignature(NaClSrpcChannel  *channel,
                                               const char       *rpc_name,
                                               ...);
/**
 *  @clientSrpc  Invokes a specified RPC on the given channel.  Parameters
 *  are passed by stdarg conventions and determined from the types specified
 *  in the method tables stored in channel.
 *  @param channel The channel descriptor to use to invoke the RPC.
 *  @param rpc_num The index of the RPC to be invoked.
 *  @return A NaClSrpcResultCodes indicating success (NACL_SRPC_RESULT_OK)
 *  or failure.
 *  @see NaClSrpcResultCodes
 */
extern NaClSrpcError NaClSrpcInvoke(NaClSrpcChannel  *channel,
                                    uint32_t         rpc_num,
                                    ...);
/**
 *  @clientSrpc Invokes a specified RPC on the given channel.  Parameters are
 *  passed in arrays and are type-checked against the types specified in the
 *  method tables stored in channel.
 *  @param channel The channel descriptor to use to invoke the RPC.
 *  @param rpc_num The index of the RPC to be invoked.
 *  @param args The array of parameter pointers to arguments to be passed in.
 *  @param rets The array of parameter pointers to arguments to be returned.
 *  @return A NaClSrpcResultCodes indicating success (NACL_SRPC_RESULT_OK)
 *  or failure.
 *  @see NaClSrpcResultCodes
 */
extern NaClSrpcError NaClSrpcInvokeV(NaClSrpcChannel *channel,
                                     uint32_t        rpc_num,
                                     NaClSrpcArg     *args[],
                                     NaClSrpcArg     *rets[]);
/**
 *  @clientSrpc Invokes a specified RPC on the given channel.  Parameters are
 *  passed in and returned through pointers to stdargs.  They are type-checked
 *  against the types specified in the method tables stored in channel.
 *  @param channel The channel descriptor to use to invoke the RPC.
 *  @param rpc_num The index of the RPC to be invoked.
 *  @param in_va The pointer to stdargs list of arguments to be passed in.
 *  @param out_va The pointer to stdargs list of arguments to be returned.
 *  @return A NaClSrpcResultCodes indicating success (NACL_SRPC_RESULT_OK)
 *  or failure.
 *  @see NaClSrpcResultCodes
 */
extern NaClSrpcError NaClSrpcInvokeVaList(NaClSrpcChannel *channel,
                                        uint32_t          rpc_num,
                                        va_list           in_va,
                                        va_list           out_va);

/**
 * The current protocol (version) number used to send and receive RPCs.
 */
static const uint32_t kNaClSrpcProtocolVersion = 0xc0da0002;

/**
 * Send an RPC request on the given channel.
 */
extern int NaClSrpcRequestWrite(NaClSrpcChannel* channel,
                                NaClSrpcRpc* rpc,
                                NaClSrpcArg* args[],
                                NaClSrpcArg* rets[]);

/**
 * Wait for a sent RPC to receive a response.
 */
extern void NaClSrpcRpcWait(NaClSrpcChannel* channel,
                            NaClSrpcRpc* rpc);

/**
 *  @serverSrpc  Returns whether the srpc server is being run "standalone";
 *  that is, not as a subprocess of sel_universal, the browser plugin, etc.
 */
int NaClSrpcIsStandalone();

/**
 * Runs a text-based interpreter given a list of SRPC methods.  This
 * calls NaClSrpcCommandLoop().
 * @return Returns zero on success, non-zero otherwise.
 */
int NaClSrpcCommandLoopMain(const struct NaClSrpcHandlerDesc *methods);

EXTERN_C_END

/**
 * @}
 * End of System Calls group
 */

#endif  /* NATIVE_CLIENT_SRC_SHARED_SRPC_NACL_SRPC_H_ */
