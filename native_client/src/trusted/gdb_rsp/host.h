/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

//  This module provides an object for handling RSP responsibilities of
//  the host side of the connection.  The host behaves like a cache, and
//  is responsible for syncronization of state between the Target and Host.
//  For example, the Host is responsible for updating the thread context
//  before restarting the Target, and for updating it's internal array of
//  threads whenever the Target stops.

#ifndef NATIVE_CLIENT_GDB_RSP_HOST_H_
#define NATIVE_CLIENT_GDB_RSP_HOST_H_ 1

#include <map>
#include <string>
#include <vector>

#include "native_client/src/trusted/port/std_types.h"

namespace gdb_rsp {

class Abi;
class Packet;
class Session;

class Host {
 public:
  enum Status {
    HS_UNINIT = -1,    // Host is uninitialized
    HS_RUNNING = 0,    // The target is running (no sig since cont.)
    HS_STOPPING = 1,   // Host has issued a break request
    HS_STOPPED = 2,    // Host has received a signal and is stopped
    HS_EXIT = 3,       // Host has received an exit code
    HS_TERMINATED = 4  // host has received a termination code
  };

  // The Host::Thread class represents a thread on the Target side, providing
  // a cache for its state, which is automatically updated by the parent Host
  // object whenever the target starts or stops.
  class Thread {
   public:
    Thread(const Abi *abi, uint32_t id);
    ~Thread();

    public:
     uint32_t GetId() const;
     const Abi *GetAbi() const;
     void GetRegister(uint32_t index, void *dst) const;
     void SetRegister(uint32_t index, const void *src);

    private:
     uint32_t id_;
     uint8_t *ctx_;
     const Abi *abi_;

    friend class Host;
  };

  typedef std::map<uint32_t, Host::Thread*> ThreadMap_t;
  typedef std::map<std::string, std::string> PropertyMap_t;
  typedef std::vector<uint32_t> ThreadVector_t;

  explicit Host(Session *session);
  ~Host();
  bool Init();

  // The following functions are provided cached values when possible.
  // For instance, GetSignal, GetThreads, and GetThread, will return
  // values that were computed during Update.
  // cause a communication between the host and target, so the public
  // functions above should be used when possible.

  // Issue a break request if the target is still running.  This is
  // asynchronous, we won't actually be "broken" until we get the signal
  bool Break();

  // Requests that we cleanly detach from the target.
  bool Detach();

  // Get the current status of the Target.
  Status  GetStatus();

  // Get the last signal (which put us into the broken state)
  int32_t GetSignal();

  // Get a list of currently active threads
  bool GetThreads(ThreadVector_t *threads) const;

  // Get a thread object by ID.
  Thread *GetThread(uint32_t id);

  // Get and set a block of target memory.
  bool GetMemory(void *dst, uint64_t addr, uint32_t size);
  bool SetMemory(const void *src, uint64_t addr, uint32_t size);

  // Read locally cached properties
  bool HasProperty(const char *name) const;
  bool ReadProperty(const char *name, std::string *val) const;

  // Read remote object
  bool ReadObject(const char *type, const char *name, std::string *val);

  // Set the SINGLE STEP flag on the current thread context, and
  // putting the target back into the RUN state.
  bool Step();

  // Issue a step request, putting us back into the RUN state.
  bool Continue();

  // Wait upto the session's packet timeout to see if we receive a break
  bool WaitForBreak();

  // The following functions are internal only and cause communication to
  // happen between the target and host.  These functions will always
  // cause a communication between the host and target, so the public
  // functions above should be used when possible.
 protected:
  // Called whenever the target transitions from running to stopped to
  // fetch information about the current state.
  bool Update();

  bool Send(Packet *req, Packet *resp);
  bool SendOnly(Packet *req);

  bool Request(const std::string &req, std::string *resp);
  bool RequestOnly(const std::string &req);
  bool RequestThreadList(ThreadVector_t *ids);

  // Parse a string, returning true and update if a valid stop packet
  bool ParseStopPacket(const char *data);

 private:
  Session *session_;
  const Abi *abi_;

  PropertyMap_t properties_;
  ThreadMap_t threads_;
  int32_t lastSignal_;
  Status  status_;
};


}  // namespace gdb_rsp

#endif  // NATIVE_CLIENT_GDB_RSP_HOST_H_

