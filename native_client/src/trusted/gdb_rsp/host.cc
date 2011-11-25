/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "native_client/src/trusted/gdb_rsp/abi.h"
#include "native_client/src/trusted/gdb_rsp/host.h"
#include "native_client/src/trusted/gdb_rsp/packet.h"
#include "native_client/src/trusted/gdb_rsp/session.h"
#include "native_client/src/trusted/gdb_rsp/util.h"

#include "native_client/src/trusted/port/std_types.h"
#include "native_client/src/trusted/port/platform.h"

#ifdef WIN32
#define snprintf sprintf_s
#endif


using std::string;
using port::IPlatform;

namespace gdb_rsp {

Host::Thread::Thread(const Abi *abi, uint32_t id)
  : id_(id), ctx_(NULL), abi_(abi) {
  ctx_ = new uint8_t[abi_->GetContextSize()];
  assert(NULL != ctx_);
}

Host::Thread::~Thread() {
  delete[] ctx_;
}

uint32_t Host::Thread::GetId() const {
  return id_;
}

const Abi *Host::Thread::GetAbi() const {
  return abi_;
}

void Host::Thread::GetRegister(uint32_t index, void *dst) const {
  assert(NULL != dst);
  assert(index < abi_->GetRegisterCount());

  const Abi::RegDef *def = abi_->GetRegisterDef(index);
  uint8_t *ptr = ctx_ + def->offset_;
  memcpy(dst, ptr, def->bytes_);
}

void Host::Thread::SetRegister(uint32_t index, const void *src) {
  assert(src);
  assert(index < abi_->GetRegisterCount());

  const Abi::RegDef *def = abi_->GetRegisterDef(index);
  uint8_t *ptr = ctx_ + def->offset_;
  memcpy(ptr, src, def->bytes_);
}


// Construct unitialzied
Host::Host(Session *session) : session_(session), abi_(NULL),
                               status_(HS_UNINIT) { }

Host::~Host() { }

bool Host::Init() {
  string reply;

  properties_.clear();

  // Set the default max packet size.  We use the default value
  // that GDB appears to like which is 1K - xsum/etc overhead.
  properties_["PacketSize"] = "3fa";

  // Get properties
  if (!Request("qSupported", &reply)) return false;

  // Parse the semicolon delimited properties
  stringvec tokens = StringSplit(reply.data(), ";");
  for (uint32_t loop = 0; loop < tokens.size(); loop++) {
    // Properties are in the form of "Key=Val", "Key+", or "Key-"
    string &prop = tokens[loop];

    // If the form is Key=Val, there should be two components Key and Val
    stringvec keyval = StringSplit(prop, "=");
    if (keyval.size() == 2) {
      properties_[keyval[0]] = keyval[1];
      continue;
    }

    // Strip off the + or - by splitting to get the name.
    keyval = StringSplit(prop, "+-");
    // Size==1 means we got either XXX, XXX+, or XXX-
    if (keyval.size() == 1) {
      size_t len = prop.length();
      // In which case the last character must be + or -, or we ignore.
      switch (prop[len - 1]) {
        case '+' :
          properties_[keyval[0]] = "true";
          continue;
        case '-' :
          properties_[keyval[0]] = "false";
          continue;
      }
    }

    // Otherwise it was a malformed property
    IPlatform::LogError("Returned feature with strange assignment: %s",
                        prop.data());
  }

  // Must support and use property read method to get CPU type
  if (!ReadObject("features", "target.xml", &reply)) return false;

  // Search for start of "architecture" tag
  const char *name = strstr(reply.data(), "<architecture>");
  if (NULL != name) {
    // Size of "<architecture>"
    const int nameStart = 14;
    char *str = strdup(&name[nameStart]);
    char *term = strchr(str, '<');
    *term = 0;

    abi_ = Abi::Find(str);
    free(str);
  }

  // Check if we failed to find the correct ABI
  if (NULL == abi_) {
    IPlatform::LogError("Failed to find ABI for %s\n", reply.data());
    return false;
  }

  return Update();
}

bool Host::Update() {
  ThreadVector_t old_ids;
  ThreadVector_t new_ids;

  ThreadMap_t::const_iterator itr = threads_.begin();
  while (itr != threads_.end()) {
    old_ids.push_back(itr->first);
    itr++;
  }

  if (!RequestThreadList(&new_ids)) return false;

  for (uint32_t loop = 0; loop < new_ids.size(); loop++) {
    // Large enough for log10(2^32) + NUL + "Hg";
    char tmp[16];

    uint32_t id = new_ids[loop];
    string request;
    string ignore_reply;

    snprintf(tmp, sizeof(tmp), "Hg%x", id);
    if (!Request(tmp, &ignore_reply)) {
      IPlatform::LogError("Failed to set thread context for %d.\n", id);
      continue;
    }

    Packet req, resp;
    req.AddString("g");
    if (!Send(&req, &resp)) {
      IPlatform::LogError("Failed to get thread registers for %d.\n", id);
      continue;
    }

    Thread *thread = threads_[id];
    if (NULL == thread) {
      thread = new Thread(abi_, id);
      threads_[id] = thread;
    }

    resp.GetBlock(thread->ctx_, abi_->GetContextSize());
  }

  // Update the current state
  string reply;
  if (Request("?", &reply)) return ParseStopPacket(reply.data());

  // If we are not "broken" then we must have failed to update
  return false;
}

bool Host::GetThreads(ThreadVector_t* threads) const {
  // We can get threads if stopped
  if (HS_STOPPED != status_) return false;

  threads->clear();
  ThreadMap_t::const_iterator itr  = threads_.begin();
  while (itr != threads_.end()) {
    threads->push_back(itr->first);
    itr++;
  }

  return true;
}

bool Host::Step() {
  Packet out;

  // We can only step if we are stopped
  if (HS_STOPPED != status_) return false;

  out.AddRawChar('s');
  if (SendOnly(&out)) {
    // We are running again (even if we expect to immediately break)
    status_ = HS_RUNNING;
    return true;
  }

  return false;
}

bool Host::Continue() {
  Packet out;

  // We can only step if we are stopped
  if (HS_STOPPED != status_) return false;

  out.AddRawChar('c');
  if (SendOnly(&out)) {
    // We are running again
    status_ = HS_RUNNING;
    return true;
  }

  return false;
}

// Wait to see if we receive a break
bool Host::WaitForBreak() {
  // We can not wait if we are not running
  if (HS_RUNNING != status_) return false;

  Packet rx;
  std::string str;
  if (session_->GetPacket(&rx) == false) return false;

  rx.GetString(&str);
  if (ParseStopPacket(str.data())) return Update();
  return false;
}



Host::Thread* Host::GetThread(uint32_t id) {
  ThreadMap_t::const_iterator itr;
  itr = threads_.find(id);

  if (itr == threads_.end()) return NULL;

  return itr->second;
}

bool Host::ParseStopPacket(const char *data) {
  if (strlen(data) < 3) return false;

  // Stop in the form of (S|T|W|X)[XX]{n=xxx,r=yyy;{...}}
  // where XX is the result code (Unix signal number for stops)
  // or the form of OXX{XX..} where XX is a hex pair encoded string.
  switch (data[0]) {
    // Both S & T signals are a normal stop
    case 'S':
    case 'T':
      status_ = HS_STOPPED;
      break;

    case 'W':
      status_ = HS_EXIT;
      break;

    case 'X':
      status_ = HS_TERMINATED;
      break;

    case 'O':
      return true;

    default:
      return false;
  }

  if (!NibblesToByte(&data[1], &lastSignal_)) return false;

  return true;
}


bool Host::RequestThreadList(ThreadVector_t* ids) {
  string reply;

  ids->clear();

  if (!Request("qfThreadInfo", &reply)) return false;

  do {
    // Check if we are done
    if (reply == "l") return true;

    if (reply[0] != 'm') {
      IPlatform::LogError("Expecting diffent thread format: %s\n",
                          reply.data());
      return false;
    }

    stringvec replys = StringSplit(&reply[1], ",");
    for (uint32_t loop = 0; loop < replys.size(); loop++) {
      ids->push_back(strtol(replys[loop].data(), NULL, 16));
    }
  } while (Request("qsThreadInfo", &reply));

  IPlatform::LogError("Failed request of qsThreadInfo.\n", reply.data());
  return false;
}

bool Host::HasProperty(const char *name) const {
  std::map<string, string>::const_iterator itr;
  itr = properties_.find(name);

  return properties_.end() != itr;
}

bool Host::ReadProperty(const char *name, string* val) const {
  std::map<string, string>::const_iterator itr;
  itr = properties_.find(name);

  if (properties_.end() == itr) return false;

  *val = itr->second;
  return true;
}


bool Host::ReadObject(const char *type, const char *name, string *reply) {
  uint32_t offset = 0;
  uint32_t maxTX = 1024;
  reply->clear();

  if (ReadProperty("PacketSize", reply)) {
    maxTX = static_cast<uint32_t>(strtol(reply->data(), NULL, 16));
  }

  while (1) {
    // We make this 64B to hold 2xlog16(2^64) + NUL + 14 characters although
    // in practice these tend to be 16b values.
    char tmp[64];

    string replyPiece;
    string query = "qXfer:";
    query += type;
    query += ":read:";
    query += name;
    snprintf(tmp, sizeof(tmp), ":%x,%x", offset, maxTX);
    query += tmp;

    if (!Request(query, &replyPiece)) return false;

    if (replyPiece[0] == 'l') {
      *reply += &replyPiece[1];
      return true;
    }

    if ((replyPiece[0] == 'E') && (replyPiece.length() == 3)) {
      return false;
    }

    if (replyPiece[0] != 'm') {
      IPlatform::LogError("Expecting more or end signal got:\n\t%s.",
                          replyPiece.data());
      return false;
    }

    *reply += replyPiece;
    offset += static_cast<uint32_t>(replyPiece.length());
  }

  return true;
}

bool Host::Break() {
  char brk[2] = { 0x03, 0x00 };

  // We can only break if running
  if (HS_RUNNING != status_) return false;

  status_ = HS_STOPPING;
  return RequestOnly(brk);
}

bool Host::Detach() {
  // We can only detach if stopped
  if (HS_STOPPED != status_) return false;

  return RequestOnly("d");
}

int32_t Host::GetSignal() {
  // We always return the lasted cached value
  return lastSignal_;
}

Host::Status Host::GetStatus() {
  return status_;
}

bool Host::GetMemory(void *dst, uint64_t addr, uint32_t size) {
  char *ptr = reinterpret_cast<char *>(dst);
  uint32_t maxTX = 1024;
  string reply;
  Packet pktReq, pktReply;

  // We can only access memory if stopped
  if (HS_STOPPED != status_) return false;

  if (ReadProperty("PacketSize", &reply)) {
    maxTX = strtoul(reply.data(), NULL, 16);
  }

  // Reply is encoded as two nibbles plus a single char
  // Mxxxxxxxx...
  maxTX = (maxTX - 1) / 2;

  while (size) {
    uint32_t len = size;
    if (len > maxTX) len = maxTX;

    pktReq.Clear();
    pktReq.AddRawChar('m');
    pktReq.AddNumberSep(addr, ',');
    pktReq.AddNumberSep(size, 0);

    if (!Send(&pktReq, &pktReply)) return false;

    if (!pktReply.GetBlock(ptr, len)) return false;

    ptr += len;
    addr += len;
    size -= len;
  }

  return true;
}

bool Host::SetMemory(const void *src, uint64_t addr, uint32_t size) {
  const char *ptr = reinterpret_cast<const char *>(src);
  uint32_t maxTX = 1024;
  string reply;
  Packet pktReq, pktReply;

  // We can only access memory if stopped
  if (HS_STOPPED != status_) return false;

  if (ReadProperty("PacketSize", &reply)) {
    maxTX = strtoul(reply.data(), NULL, 16);
  }

  // Reply is encoded as two nibbles plus a single char(3), plus address (16)
  // a and size (8) or Maaaaaaaaaaaaaaaa,ssssssss:(27)
  maxTX = (maxTX - 27) / 2;

  while (size) {
    uint32_t len = size;
    if (len > maxTX) len = maxTX;

    pktReq.Clear();
    pktReq.AddRawChar('M');
    pktReq.AddNumberSep(addr, ',');
    pktReq.AddNumberSep(len, ':');
    pktReq.AddBlock(src, len);

    if (!Send(&pktReq, &pktReply)) return false;

    ptr += len;
    addr += len;
    size -= len;
  }

  return true;
}

bool Host::RequestOnly(const string& req) {
  Packet pktReq;

  pktReq.AddString(req.data());
  return SendOnly(&pktReq);
}

bool Host::Request(const string& req, string *resp) {
  Packet pktReq, pktResp;

  pktReq.AddString(req.data());
  bool result = Send(&pktReq, &pktResp);

  pktResp.GetString(resp);

  // Check for error code on return
  if ((resp->length() == 3) && (resp->data()[0] == 'E')) return false;

  return result;
}

bool Host::SendOnly(Packet *tx) {
  return session_->SendPacketOnly(tx);
}

bool Host::Send(Packet *tx, Packet *rx) {
  if (!session_->SendPacket(tx)) return false;
  return session_->GetPacket(rx);
}

}  // namespace gdb_rsp


