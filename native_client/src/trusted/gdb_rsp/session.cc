/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <string>
#include <sstream>

#include "native_client/src/trusted/gdb_rsp/packet.h"
#include "native_client/src/trusted/gdb_rsp/session.h"
#include "native_client/src/trusted/gdb_rsp/util.h"

#include "native_client/src/trusted/port/mutex.h"
#include "native_client/src/trusted/port/platform.h"
#include "native_client/src/trusted/port/transport.h"

using port::IPlatform;
using port::ITransport;
using port::IMutex;
using port::MutexLock;

// Use a timeout of 1 second
int const kSessionTimeoutMs = 1000;

namespace gdb_rsp {

Session::Session()
  : mutex_(NULL),
    io_(NULL),
    flags_(0),
    seq_(0),
    connected_(false) {
}

Session::~Session() {
  if (mutex_) IMutex::Free(mutex_);
}


bool Session::Init(port::ITransport *transport) {
  if (NULL == transport) return false;

  mutex_ = IMutex::Allocate();
  if (NULL == mutex_) return false;

  connected_ = true;
  io_ = transport;
  return true;
}

void Session::SetFlags(uint32_t flags) {
  flags_ |= flags;
}

void Session::ClearFlags(uint32_t flags) {
  flags_ &= ~flags;
}

uint32_t Session::GetFlags() {
  return flags_;
}

bool Session::DataAvailable() {
  assert(io_);

  return io_->ReadWaitWithTimeout(kSessionTimeoutMs);
}

bool Session::Connected() {
  return connected_;
}

bool Session::GetChar(char *ch) {
  assert(io_);

  // Attempt to select this IO for reading.
  if (DataAvailable() == false) return false;

  int32_t len = io_->Read(ch, 1);

  // If data is "availible" but we can't read, it must be closed.
  if (len < 1) {
    io_->Disconnect();
    connected_ = false;
    return false;
  }

  return true;
}


bool Session::SendPacket(Packet *pkt) {
  MutexLock lock(mutex_);
  char ch;

  do {
    if (!SendPacketOnly(pkt)) return false;

    // If ACKs are off, we are done.
    if (GetFlags() & IGNORE_ACK) break;

    // Otherwise, poll for '+'
    if (!GetChar(&ch)) return false;

    // Retry if we didn't get a '+'
  } while (ch != '+');

  return true;
}


bool Session::SendPacketOnly(Packet *pkt) {
  MutexLock lock(mutex_);

  const char *ptr;
  char ch;
  std::stringstream outstr;

  char run_xsum = 0;
  int32_t seq;

  ptr = pkt->GetPayload();

  if (!pkt->GetSequence(&seq) && (GetFlags() & USE_SEQ)) {
    pkt->SetSequence(seq_++);
  }

  // Signal start of response
  outstr << '$';

  // If there is a sequence, send as two nibble 8bit value + ':'
  if (pkt->GetSequence(&seq)) {
    IntToNibble((seq & 0xFF) >> 4, &ch);
    outstr << ch;
    run_xsum += ch;

    IntToNibble(seq & 0xF, &ch);
    outstr << ch;
    run_xsum += ch;

    ch = ':';
    outstr << ch;
    run_xsum += ch;
  }

  // Send the main payload
  int offs = 0;
  while ((ch = ptr[offs++]) != 0) {
    outstr << ch;
    run_xsum += ch;
  }

  // Send XSUM as two nible 8bit value preceeded by '#'
  outstr << '#';
  IntToNibble((run_xsum >> 4) & 0xF, &ch);
  outstr << ch;
  IntToNibble(run_xsum & 0xF, &ch);
  outstr << ch;

  return SendStream(outstr.str().data());
}

// We do not take the mutex here since we already have it
// this function is protected so it can't be called directly.
bool Session::SendStream(const char *out) {
  int32_t len = static_cast<int32_t>(strlen(out));
  int32_t sent = 0;

  assert(io_);

  while (sent < len) {
    const char *cur = &out[sent];
    int32_t tx = io_->Write(cur, len - sent);

    if (tx <= 0) {
      IPlatform::LogWarning("Send of %d bytes : '%s' failed.\n", len, out);
      io_->Disconnect();
      connected_ = false;
      return false;
    }

    sent += tx;
  }

  if (GetFlags() & DEBUG_SEND) IPlatform::LogInfo("TX %s\n", out);
  return true;
}


// Attempt to receive a packet
bool Session::GetPacket(Packet *pkt) {
  assert(io_);

  MutexLock lock(mutex_);

  char run_xsum, fin_xsum, ch;
  std::string in;
  int has_seq, offs;

  // If nothing is waiting, return false
  if (!io_->ReadWaitWithTimeout(kSessionTimeoutMs)) return false;

  // Toss characters until we see a start of command
  do {
    if (!GetChar(&ch)) return false;
    in += ch;
  } while (ch != '$');

 retry:
  has_seq = 1;
  offs    = 0;

  // If nothing is waiting, return NONE
  if (!io_->ReadWaitWithTimeout(kSessionTimeoutMs)) return false;

  // Clear the stream
  pkt->Clear();

  // Prepare XSUM calc
  run_xsum = 0;
  fin_xsum = 0;

  // Stream in the characters
  while (1) {
    if (!GetChar(&ch)) return false;

    in += ch;
     // Check SEQ statemachine  xx:
    switch (offs) {
      case 0:
      case 1:
        if (!NibbleToInt(ch, 0)) has_seq = 0;
        break;

      case 2:
        if (ch != ':') has_seq = 0;
        break;
    }
    offs++;

    // If we see a '#' we must be done with the data
    if (ch == '#') break;

    // If we see a '$' we must have missed the last cmd
    if (ch == '$') {
      IPlatform::LogInfo("RX Missing $, retry.\n");
      goto retry;
    }
    // Keep a running XSUM
    run_xsum += ch;
    pkt->AddRawChar(ch);
  }


  // Get two Nibble XSUM
  if (!GetChar(&ch)) return false;
  in += ch;

  int val;
  NibbleToInt(ch, & val);
  fin_xsum = val << 4;

  if (!GetChar(&ch)) return false;
  in += ch;
  NibbleToInt(ch, &val);
  fin_xsum |= val;

  if (GetFlags() & DEBUG_RECV) IPlatform::LogInfo("RX %s\n", in.data());

  // Pull off teh sequence number if we have one
  if (has_seq) {
    uint8_t seq;
    char ch;

    pkt->GetWord8(&seq);
    pkt->SetSequence(seq);
    pkt->GetRawChar(&ch);
    if (ch != ':') {
      IPlatform::LogError("RX mismatched SEQ.\n");
      return false;
    }
  }

  // If ACKs are off, we are done.
  if (GetFlags() & IGNORE_ACK) return true;

  // If the XSUMs don't match, signal bad packet
  if (fin_xsum == run_xsum) {
    char out[4] = { '+', 0, 0, 0};
    int32_t seq;

    // If we have a sequence number
    if (pkt->GetSequence(&seq)) {
      // Respond with Sequence number
      IntToNibble(seq >> 4, &out[1]);
      IntToNibble(seq & 0xF, &out[2]);
    }
    return SendStream(out);
  } else {
    // Resend a bad XSUM and look for retransmit
    SendStream("-");

    IPlatform::LogInfo("RX Bad XSUM, retry\n");
    goto retry;
  }

  return true;
}

}  // End of namespace gdb_rsp

