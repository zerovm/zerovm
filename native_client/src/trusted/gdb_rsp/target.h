/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */


// This module provides interfaces for accessing the debugging state of
// the target.  The target can use either the thread that took the
// exception or run in it's own thread.  To respond to the host, the
// application must call the run function with a valid Transport
// which will then be polled for commands.  The target will return
// from Run when the host disconnects, or requests a continue.
//
// The object is protected by a mutex, so that it is legal to track or
// ignore threads as an exception takes place.
//
// The Run function expects that all threads of interest are stopped
// with the Step flag cleared before Run is called.  It is expected that
// and that all threads are updated with thier modified contexts and
// restarted when the target returns from Run.

#ifndef SRC_TRUSTED_GDB_RSP_TARGET_H_
#define SRC_TRUSTED_GDB_RSP_TARGET_H_ 1

#include <map>
#include <string>

#include "native_client/src/trusted/gdb_rsp/util.h"

#include "native_client/src/trusted/port/event.h"
#include "native_client/src/trusted/port/platform.h"
#include "native_client/src/trusted/port/mutex.h"
#include "native_client/src/trusted/port/thread.h"

namespace gdb_rsp {

class Abi;
class Packet;
class Session;

class Target {
 public:
  enum ErrDef {
    NONE = 0,
    BAD_FORMAT = 1,
    BAD_ARGS = 2,
    FAILED = 3
  };

  enum State {
    UNINIT = 0,
    RUNNING = 1,
    STOPPED = 2
  };

  typedef ErrDef (*QFunc_t)(Target *, stringvec&, std::string);
  typedef std::map<uint32_t, port::IThread*> ThreadMap_t;
  typedef std::map<std::string, std::string> PropertyMap_t;
  typedef std::map<uint64_t, uint8_t*> BreakMap_t;

 public:
  // Contruct a Target object.  By default use the native ABI.
  explicit Target(const Abi *abi = NULL);
  ~Target();

  // Init must be the first function called to correctlty
  // build the Target internal structures.
  bool Init();

  void SetMemoryBase(uint64_t mem_base) { mem_base_ = mem_base; }

  // Add and remove temporary breakpoints.  These breakpoints
  // must be added just before we start running, and removed
  // just before we stop running to prevent the debugger from
  // seeing the modified memory.
  bool AddTemporaryBreakpoint(uint64_t address);
  bool RemoveTemporaryBreakpoints();

  // This function should be called by a tracked thread when it takes
  // an exception.  It takes sig_start_ to prevent other exceptions
  // from signalling thread.  If wait is true, it will then  block on
  // sig_done_ until a continue is issued by the host.
  void Signal(uint32_t id, int8_t sig, bool wait);

  // This function will spin on a session, until it closes.  If an
  // exception is caught, it will signal the exception thread by
  // setting sig_done_.
  void Run(Session *ses);

  // This function causes the target to track the state
  // of the specified thread and make it availible to
  // a connected host.
  void TrackThread(port::IThread *thread);

  // This function causes the target to stop tracking the
  // state of the specified thread, which will no longer
  // be visible to the host.
  void IgnoreThread(port::IThread *thread);

 protected:
  // This function always succeedes, since all errors
  // are reported as an error string of "E<##>" where
  // the two digit number.  The error codes are not
  // not documented, so this implementation uses
  // ErrDef as errors codes.  This function returns
  // true a request to continue (or step) is processed.
  bool ProcessPacket(Packet *pktIn, Packet *pktOut);

  void Destroy();
  void Detach();

  bool GetFirstThreadId(uint32_t *id);
  bool GetNextThreadId(uint32_t *id);

  uint32_t GetRegThreadId() const;
  uint32_t GetRunThreadId() const;
  port::IThread *GetThread(uint32_t id);

 public:
  const Abi *abi_;

  // This mutex protects debugging state (threads_, cur_signal, sig_thread_)
  port::IMutex *mutex_;

  // This event is signalled when the target is really to process an
  // exception.  It ensures only one exception is processed at a time.
  port::IEvent *sig_start_;

  // This event is signalled when the target done processing an exception.
  port::IEvent *sig_done_;

  // This value is set if the exception cather is requesting a continue signal
  bool send_done_;

  ThreadMap_t threads_;
  ThreadMap_t::const_iterator threadItr_;
  BreakMap_t breakMap_;


  PropertyMap_t properties_;

  uint8_t *ctx_;         // Context Scratchpad

  // The current signal and signaling thread data is protected by
  // the mutex, and can only be owned by one thread at a time.
  // These values should only be written via the "Signal" member,
  // which will ensure that a new signal does not overwrite a
  // previous signal.
  volatile int8_t cur_signal_;
  volatile uint32_t sig_thread_;

  uint32_t run_thread_;   // Which thread to issue step commands on
  uint32_t reg_thread_;   // Which thread to issue context (reg) commands on
  uint64_t mem_base_;
};


}  // namespace gdb_rsp

#endif  // SRC_TRUSTED_GDB_RSP_TARGET_H_

