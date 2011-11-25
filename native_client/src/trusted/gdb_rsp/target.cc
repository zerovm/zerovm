/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "native_client/src/trusted/gdb_rsp/abi.h"
#include "native_client/src/trusted/gdb_rsp/packet.h"
#include "native_client/src/trusted/gdb_rsp/target.h"
#include "native_client/src/trusted/gdb_rsp/session.h"
#include "native_client/src/trusted/gdb_rsp/util.h"

#include "native_client/src/trusted/port/platform.h"
#include "native_client/src/trusted/port/thread.h"

#ifdef WIN32
#define snprintf sprintf_s
#endif

using std::string;

using port::IEvent;
using port::IMutex;
using port::IPlatform;
using port::IThread;
using port::MutexLock;

namespace gdb_rsp {


Target::Target(const Abi* abi)
  : abi_(abi),
    mutex_(NULL),
    sig_start_(NULL),
    sig_done_(NULL),
    send_done_(false),
    ctx_(NULL),
    cur_signal_(-1),
    sig_thread_(0),
    run_thread_(-1),
    reg_thread_(-1),
    mem_base_(0) {
  if (NULL == abi_) abi_ = Abi::Get();
}

Target::~Target() {
  Destroy();
}

bool Target::Init() {
  string targ_xml = "l<target><architecture>";

  targ_xml += abi_->GetName();
  targ_xml += "</architecture></target>";

  // Set a more specific result which won't change.
  properties_["target.xml"] = targ_xml;
  properties_["Supported"] =
    "PacketSize=7cf;qXfer:libraries:read+;qXfer:features:read+";

  mutex_ = IMutex::Allocate();
  sig_start_ = IEvent::Allocate();
  sig_done_ = IEvent::Allocate();
  ctx_ = new uint8_t[abi_->GetContextSize()];

  if ((NULL == mutex_) || (NULL == sig_start_) || (NULL == sig_done_)
      || (NULL == ctx_)) {
    Destroy();
    return false;
  }

  // Allow one exception to happen
  sig_start_->Signal();
  return true;
}

void Target::Destroy() {
  if (mutex_) IMutex::Free(mutex_);
  if (sig_start_) IEvent::Free(sig_start_);
  if (sig_done_) IEvent::Free(sig_done_);

  delete[] ctx_;
}

bool Target::AddTemporaryBreakpoint(uint64_t address) {
  const Abi::BPDef *bp = abi_->GetBreakpointDef();

  // If this ABI does not support breakpoints then fail
  if (NULL == bp) return false;

  // If we alreay have a breakpoint here then don't add it
  BreakMap_t::iterator itr = breakMap_.find(address);
  if (itr != breakMap_.end()) return false;

  uint8_t *data = new uint8_t[bp->size_];
  if (NULL == data) return false;

  // Copy the old code from here
  if (IPlatform::GetMemory(address, bp->size_, data) == false) {
    delete[] data;
    return false;
  }
  if (IPlatform::SetMemory(address, bp->size_, bp->code_) == false) {
    delete[] data;
    return false;
  }

  breakMap_[address] = data;
  return true;
}

bool Target::RemoveTemporaryBreakpoints() {
  const Abi::BPDef *bp = abi_->GetBreakpointDef();

  // Iterate through the map, removing breakpoints
  while (!breakMap_.empty()) {
    // Copy the key/value locally
    BreakMap_t::iterator cur = breakMap_.begin();
    uint64_t addr = cur->first;
    uint8_t *data = cur->second;

    // Then remove it from the map
    breakMap_.erase(cur);

    // Copy back the old code, and free the data
    if (!IPlatform::SetMemory(addr, bp->size_, data))
      port::IPlatform::LogError("Failed to undo breakpoint.\n");
    delete[] data;
  }

  return true;
}



void Target::Signal(uint32_t id, int8_t sig, bool wait) {
  // Wait for this signal's turn in the signal Q.
  sig_start_->Wait();
  {
    // Now lock the target, sleeping all active threads
    MutexLock lock(mutex_);

    // Suspend all threads except this one
    uint32_t curId;
    bool more = GetFirstThreadId(&curId);
    while (more) {
      if (curId != id) {
        IThread *thread = threads_[curId];
        thread->Suspend();
      }
      more = GetNextThreadId(&curId);
    }

    // Signal the stub (Run thread) that we are ready to process
    // a trap, by updating the signal information and releasing
    // the lock.
    reg_thread_ = id;
    run_thread_ = id;
    cur_signal_ = sig;
  }

  // Wait for permission to continue
  if (wait) sig_done_->Wait();
}

void Target::Run(Session *ses) {
  bool first = true;
  do {
    // Give everyone else a chance to use the lock
    IPlatform::Relinquish(100);

    // Lock to prevent anyone else from modifying threads
    // or updating the signal information.
    MutexLock lock(mutex_);
    Packet recv, reply;

    uint32_t id = 0;

    // If no signal is waiting for this iteration...
    if (-1 == cur_signal_) {
      // but the debugger is talking to us then force a break
      if (ses->DataAvailable()) {
        // set signal to 0 to signify paused
        cur_signal_ = 0;

        // put all the threads to sleep.
        uint32_t curId;
        bool more = GetFirstThreadId(&curId);
        while (more) {
          if (curId != id) {
            IThread *thread = threads_[curId];
            thread->Suspend();
          }
          more = GetNextThreadId(&curId);
        }
      } else {
        // otherwise, nothing to do so try again.
        continue;
      }
    } else {
      // otherwise there really is an exception so get the id of the thread
      id = GetRegThreadId();
    }

    // If we got this far, then there is some kind of signal.
    // So first, remove the breakpoints
    RemoveTemporaryBreakpoints();

    // Next update the current thread info
    char tmp[16];
    snprintf(tmp, sizeof(tmp), "QC%x", id);
    properties_["C"] = tmp;

    if (first) {
      // First time on a connection, we don't sent the signal
      first = false;
    } else {
      // All other times, send the signal that triggered us
      Packet pktOut;
      pktOut.AddRawChar('S');
      pktOut.AddWord8(cur_signal_);
      ses->SendPacketOnly(&pktOut);
    }

    // Now we are ready to process commands
    // Loop through packets until we process a continue
    // packet.
    do {
      if (ses->GetPacket(&recv)) {
        reply.Clear();
        if (ProcessPacket(&recv, &reply)) {
          // If this is a continue command, break out of this loop
          break;
        } else {
          // Othwerise send the reponse
          ses->SendPacket(&reply);
        }
      }
    } while (ses->Connected());


    // Now that we are done, we want to continue in the "correct order".
    // This means letting the active thread go first, in case we are single
    // stepping and want to catch it again.  This is a desired behavior but
    // it is not guaranteed since another thread may already be in an
    // exception state and next in line to notify the target.

    // If the run thread is not the exception thread, wake it up now.
    uint32_t run_thread = GetRunThreadId();
    if (run_thread != id
        && run_thread != static_cast<uint32_t>(-1)) {
      IThread* thread = threads_[run_thread];
      thread->Resume();
    }

    // Next, wake up the exception thread, if there is one and it needs
    // to wake up.
    if (id && send_done_) sig_done_->Signal();

    // Now wake up everyone else
    uint32_t curId;
    bool more = GetFirstThreadId(&curId);
    while (more) {
      if ((curId != id) && (curId != GetRunThreadId())) {
        IThread *thread = threads_[curId];
        thread->Resume();
      }
      more = GetNextThreadId(&curId);
    }

    // Reset the signal value
    cur_signal_ = -1;

    // If we took an exception, let the handler resume and allow
    // the next exception to come in.
    if (cur_signal_) {
      sig_done_->Signal();
      sig_start_->Signal();
    }

    // Continue running until the connection is lost.
  } while (ses->Connected());
}




bool Target::GetFirstThreadId(uint32_t *id) {
  threadItr_ = threads_.begin();
  return GetNextThreadId(id);
}

bool Target::GetNextThreadId(uint32_t *id) {
  if (threadItr_ == threads_.end()) return false;

  *id = (*threadItr_).first;
  threadItr_++;

  return true;
}



bool Target::ProcessPacket(Packet* pktIn, Packet* pktOut) {
  char cmd;
  int32_t seq = -1;
  ErrDef  err = NONE;

  // Clear the outbound message
  pktOut->Clear();

  // Pull out the sequence.
  pktIn->GetSequence(&seq);
  if (seq != -1) pktOut->SetSequence(seq);

  // Find the command
  pktIn->GetRawChar(&cmd);

  switch (cmd) {
    // IN : $?
    // OUT: $Sxx
    case '?':
      pktOut->AddRawChar('S');
      pktOut->AddWord8(cur_signal_);
      break;

    // IN : $d
    // OUT: -NONE-
    case 'd':
      Detach();
      break;

    // IN : $g
    // OUT: $xx...xx
    case 'g': {
      uint32_t id = GetRegThreadId();
      if (0 == id) {
        err = BAD_ARGS;
        break;
      }

      IThread *thread = GetThread(id);
      if (NULL == thread) {
        err = BAD_ARGS;
        break;
      }

      // Copy OS preserved registers to GDB payload
      for (uint32_t a = 0; a < abi_->GetRegisterCount(); a++) {
        const Abi::RegDef *def = abi_->GetRegisterDef(a);
        thread->GetRegister(a, &ctx_[def->offset_], def->bytes_);
      }

      pktOut->AddBlock(ctx_, abi_->GetContextSize());
      break;
    }

    // IN : $Gxx..xx
    // OUT: $OK
    case 'G': {
      uint32_t id = GetRegThreadId();
      if (0 == id) {
        err = BAD_ARGS;
        break;
      }

      IThread *thread = threads_[id];
      if (NULL == thread) {
        err = BAD_ARGS;
        break;
      }

      // GDB payload to OS registers
      for (uint32_t a = 0; a < abi_->GetRegisterCount(); a++) {
        const Abi::RegDef *def = abi_->GetRegisterDef(a);
        thread->SetRegister(a, &ctx_[def->offset_], def->bytes_);
      }
      pktOut->AddBlock(ctx_, abi_->GetContextSize());
      break;
    }

    // IN : $H(c/g)(-1,0,xxxx)
    // OUT: $OK
    case 'H': {
        char type;
        uint64_t id;

        if (!pktIn->GetRawChar(&type)) {
          err = BAD_FORMAT;
          break;
        }
        if (!pktIn->GetNumberSep(&id, 0)) {
          err = BAD_FORMAT;
          break;
        }

        if (threads_.begin() == threads_.end()) {
            err = BAD_ARGS;
            break;
        }

        // If we are using "any" get the first thread
        if (id == static_cast<uint64_t>(-1)) id = threads_.begin()->first;

        // Verify that we have the thread
        if (threads_.find(static_cast<uint32_t>(id)) == threads_.end()) {
          err = BAD_ARGS;
          break;
        }

        pktOut->AddString("OK");
        switch (type) {
          case 'g':
            reg_thread_ = static_cast<uint32_t>(id);
            break;

          case 'c':
            run_thread_ = static_cast<uint32_t>(id);
            break;

          default:
            err = BAD_ARGS;
            break;
        }
        break;
      }

    // IN : $maaaa,llll
    // OUT: $xx..xx
    case 'm': {
        uint64_t addr;
        uint64_t wlen;
        uint32_t len;
        if (!pktIn->GetNumberSep(&addr, 0)) {
          err = BAD_FORMAT;
          break;
        }
        if (addr < mem_base_) {
          addr += mem_base_;
        }
        if (!pktIn->GetNumberSep(&wlen, 0)) {
          err = BAD_FORMAT;
          break;
        }

        len = static_cast<uint32_t>(wlen);
        uint8_t *block = new uint8_t[len];
        if (!port::IPlatform::GetMemory(addr, len, block)) err = FAILED;

        pktOut->AddBlock(block, len);
        break;
      }

    // IN : $Maaaa,llll:xx..xx
    // OUT: $OK
    case 'M':  {
        uint64_t addr;
        uint64_t wlen;
        uint32_t len;

        if (!pktIn->GetNumberSep(&addr, 0)) {
          err = BAD_FORMAT;
          break;
        }
        if (addr < mem_base_) {
          addr += mem_base_;
        }

        if (!pktIn->GetNumberSep(&wlen, 0)) {
          err = BAD_FORMAT;
          break;
        }

        len = static_cast<uint32_t>(wlen);
        uint8_t *block = new uint8_t[len];
        pktIn->GetBlock(block, len);

        if (!port::IPlatform::SetMemory(addr, len, block)) err = FAILED;

        pktOut->AddString("OK");
        break;
      }

    case 'q': {
      string tmp;
      const char *str = &pktIn->GetPayload()[1];
      stringvec toks = StringSplit(str, ":;");
      PropertyMap_t::const_iterator itr = properties_.find(toks[0]);

      // If this is a thread query
      if (!strcmp(str, "fThreadInfo") || !strcmp(str, "sThreadInfo")) {
        uint32_t curr;
        bool more = false;
        if (str[0] == 'f') {
          more = GetFirstThreadId(&curr);
        } else {
          more = GetNextThreadId(&curr);
        }

        if (!more) {
          pktOut->AddString("l");
        } else {
          pktOut->AddString("m");
          pktOut->AddNumberSep(curr, 0);
        }
        break;
      }

      // Check for architecture query
      tmp = "Xfer:features:read:target.xml";
      if (!strncmp(str, tmp.data(), tmp.length())) {
        stringvec args = StringSplit(&str[tmp.length()+1], ",");
        if (args.size() != 2) break;

        const char *out = properties_["target.xml"].data();
        int offs = strtol(args[0].data(), NULL, 16);
        int max  = strtol(args[1].data(), NULL, 16) + offs;
        int len  = static_cast<int>(strlen(out));

        if (max >= len) max = len;

        while (offs < max) {
          pktOut->AddRawChar(out[offs]);
          offs++;
        }
        break;
      }

      // Check the property cache
      if (itr != properties_.end()) {
        pktOut->AddString(itr->second.data());
      }
      break;
    }

    case 'T': {
      uint64_t id;
      if (!pktIn->GetNumberSep(&id, 0)) {
        err = BAD_FORMAT;
        break;
      }

      if (GetThread(static_cast<uint32_t>(id)) == NULL) {
        err = BAD_ARGS;
        break;
      }

      pktOut->AddString("OK");
      break;
    }

    case 's':  {
      IThread *thread = GetThread(GetRunThreadId());
      if (thread) thread->SetStep(true);
      return true;
    }

    case 'c':
      return true;

    default: {
      // If the command is not recognzied, ignore it by sending an
      // empty reply.
      string str;
      pktIn->GetString(&str);
      port::IPlatform::LogError("Unknown command: %s\n", str.data());
      return false;
    }
  }

  // If there is an error, return the error code instead of a payload
  if (err) {
    pktOut->Clear();
    pktOut->AddRawChar('E');
    pktOut->AddWord8(err);
  }
  return false;
}


void Target::TrackThread(IThread* thread) {
  uint32_t id = thread->GetId();
  mutex_->Lock();
  threads_[id] = thread;
  mutex_->Unlock();
}

void Target::IgnoreThread(IThread* thread) {
  uint32_t id = thread->GetId();
  mutex_->Lock();
  ThreadMap_t::iterator itr = threads_.find(id);

  if (itr != threads_.end()) threads_.erase(itr);
  mutex_->Lock();
}


void Target::Detach() {
  port::IPlatform::LogInfo("Requested Detach.\n");
}


uint32_t Target::GetRegThreadId() const {
  ThreadMap_t::const_iterator itr;

  switch (reg_thread_) {
    // If we wany "any" then try the signal'd thread first
    case 0:
    case 0xFFFFFFFF:
      itr = threads_.begin();
      break;

    default:
      itr = threads_.find(reg_thread_);
      break;
  }

  if (itr == threads_.end()) return 0;

  return itr->first;
}

uint32_t Target::GetRunThreadId() const {
  return run_thread_;
}

IThread* Target::GetThread(uint32_t id) {
  ThreadMap_t::const_iterator itr;
  itr = threads_.find(id);
  if (itr != threads_.end()) return itr->second;

  return NULL;
}


}  // namespace gdb_rsp




