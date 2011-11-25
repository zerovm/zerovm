/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifdef _WIN32
#include <windows.h>
#ifndef AF_IPX
#include <winsock2.h>
#endif
#define SOCKET_HANDLE SOCKET
#else

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SOCKET_HANDLE int
#define closesocket close
#endif

#include <stdlib.h>
#include <string>

#include "native_client/src/trusted/gdb_rsp/util.h"
#include "native_client/src/trusted/port/platform.h"
#include "native_client/src/trusted/port/transport.h"

using gdb_rsp::stringvec;
using gdb_rsp::StringSplit;

namespace port {

typedef int socklen_t;

class Transport : public ITransport {
 public:
  Transport() {
    handle_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  }

  explicit Transport(SOCKET_HANDLE s) {
    handle_ = s;
  }

  ~Transport() {
    if (handle_ != -1) closesocket(handle_);
  }

  // Read from this transport, return a negative value if there is an error
  // otherwise return the number of bytes actually read.
  virtual int32_t Read(void *ptr, int32_t len) {
    return ::recv(handle_, reinterpret_cast<char *>(ptr), len, 0);
  }

  // Write to this transport, return a negative value if there is an error
  // otherwise return the number of bytes actually written.
  virtual int32_t Write(const void *ptr, int32_t len) {
    return ::send(handle_, reinterpret_cast<const char *>(ptr), len, 0);
  }

  // Return true if data becomes availible or false after ms milliseconds.
  virtual bool ReadWaitWithTimeout(uint32_t ms = 0) {
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(handle_, &fds);

    // We want a "non-blocking" check
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec= ms * 1000;

    // Check if this file handle can select on read
    int cnt = select(static_cast<int>(handle_) + 1, &fds, 0, 0, &timeout);

    // If we are ready, or if there is an error.  We return true
    // on error, to "timeout" and let the next IO request fail.
    if (cnt != 0) return true;

    return false;
  }

// On windows, the header that defines this has other definition
// colitions, so we define it outselves just in case
#ifndef SD_BOTH
#define SD_BOTH 2
#endif

  virtual void Disconnect() {
    // Shutdown the conneciton in both diections.  This should
    // always succeed, and nothing we can do if this fails.
    (void) ::shutdown(handle_, SD_BOTH);
  }

 protected:
  SOCKET_HANDLE handle_;
};

// Convert string in the form of [addr][:port] where addr is a
// IPv4 address or host name, and port is a 16b tcp/udp port.
// Both portions are optional, and only the portion of the address
// provided is updated.  Values are provided in network order.
static bool StringToIPv4(const std::string &instr, uint32_t *addr,
                         uint16_t *port) {
  // Make a copy so the are unchanged unless we succeed
  uint32_t outaddr = *addr;
  uint16_t outport = *port;

  // Substrings of the full ADDR:PORT
  std::string addrstr;
  std::string portstr;

  // We should either have one or two tokens in the form of:
  //  IP - IP, NUL
  //  IP: -  IP, NUL
  //  :PORT - NUL, PORT
  //  IP:PORT - IP, PORT

  // Search for the port marker
  size_t portoff = instr.find(':');

  // If we found a ":" before the end, get both substrings
  if ((portoff != std::string::npos) && (portoff + 1 < instr.size())) {
    addrstr = instr.substr(0, portoff);
    portstr = instr.substr(portoff + 1, std::string::npos);
  } else {
    // otherwise the entire string is the addr portion.
    addrstr = instr;
    portstr = "";
  }

  // If the address portion was provided, update it
  if (addrstr.size()) {
    // Special case 0.0.0.0 which means any IPv4 interface
    if (addrstr == "0.0.0.0") {
      outaddr = 0;
    } else {
      struct hostent *host = gethostbyname(addrstr.data());

      // Check that we found an IPv4 host
      if ((NULL == host) || (AF_INET != host->h_addrtype))  return false;

      // Make sure the IP list isn't empty.
      if (0 == host->h_addr_list[0]) return false;

      // Use the first address in the array of address pointers.
      uint32_t **addrarray = reinterpret_cast<uint32_t**>(host->h_addr_list);
      outaddr = *addrarray[0];
    }
  }

  // if the port portion was provided, then update it
  if (portstr.size()) {
    int val = atoi(portstr.data());
    if ((val < 0) || (val > 65535)) return false;
    outport = ntohs(static_cast<uint16_t>(val));
  }

  // We haven't failed, so set the values
  *addr = outaddr;
  *port = outport;
  return true;
}


static SOCKET_HANDLE s_ServerSock;

static bool SocketInit() {
  s_ServerSock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s_ServerSock == -1) {
    IPlatform::LogError("Failed to create socket.\n");
    return false;
  }

  return true;
}

static bool SocketsAvailable() {
  static bool _init = SocketInit();
  return _init;
}

static bool BuildSockAddr(const char *addr, struct sockaddr_in *sockaddr) {
  std::string addrstr = addr;
  uint32_t *pip = reinterpret_cast<uint32_t*>(&sockaddr->sin_addr.s_addr);
  uint16_t *pport = reinterpret_cast<uint16_t*>(&sockaddr->sin_port);

  sockaddr->sin_family = AF_INET;
  return StringToIPv4(addrstr, pip, pport);
}

ITransport* ITransport::Connect(const char *addr) {
  if (!SocketsAvailable()) return NULL;

  SOCKET_HANDLE s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (s == -1) {
    IPlatform::LogError("Failed to create connection socket.\n");
    return NULL;
  }

  struct sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_addr.s_addr = htonl(0x7F000001);
  saddr.sin_port = htons(4014);

  // Override portions address that are provided
  if (addr) BuildSockAddr(addr, &saddr);

  if (::connect(s, reinterpret_cast<sockaddr*>(&saddr), sizeof(saddr)) != 0) {
    IPlatform::LogError("Failed to connect.\n");
    return NULL;
  }

  return new Transport(s);
}

ITransport* ITransport::Accept(const char *addr) {
  static bool listening = false;
  if (!SocketsAvailable()) return NULL;

  if (!listening) {
    struct sockaddr_in saddr;
    socklen_t addrlen = static_cast<socklen_t>(sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(0x7F000001);
    saddr.sin_port = htons(4014);

    // Override portions address that are provided
    if (addr) BuildSockAddr(addr, &saddr);

    // This is necessary to ensure that the TCP port is released
    // promptly when sel_ldr exits.  Without this, the TCP port might
    // only be released after a timeout, and later processes can fail
    // to bind it.
    int reuse_address = 1;
    setsockopt(s_ServerSock, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<char *>(&reuse_address),
               sizeof(reuse_address));

    struct sockaddr *psaddr = reinterpret_cast<struct sockaddr *>(&saddr);
    if (bind(s_ServerSock, psaddr, addrlen)) {
      IPlatform::LogError("Failed to bind server.\n");
      return NULL;
    }

    if (listen(s_ServerSock, 1)) {
      IPlatform::LogError("Failed to listen.\n");
      return NULL;
    }

    listening = true;
  }

  if (listening) {
    SOCKET_HANDLE s = ::accept(s_ServerSock, NULL, 0);
    if (-1 != s) return new Transport(s);
    return NULL;
  }

  return NULL;
}

void ITransport::Free(ITransport* itrans) {
  Transport* trans = static_cast<Transport*>(itrans);
  delete trans;
}

}  // namespace port

