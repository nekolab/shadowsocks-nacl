/*
 * Copyright (C) 2016  Sunny <ratsunny@gmail.com>
 *
 * This file is part of Shadowsocks-NaCl.
 *
 * Shadowsocks-NaCl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Shadowsocks-NaCl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SS_UDP_RELAY_HANDLER_H_
#define _SS_UDP_RELAY_HANDLER_H_

#include <ctime>
#include <map>
#include <list>
#include <utility>
#include "ppapi/cpp/udp_socket.h"
#include "ppapi/cpp/net_address.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "socks5.h"
#include "encrypt.h"

class Local;
class SSInstance;
class TCPRelayHandler;

class UDPRelayHandler {
 public:
  UDPRelayHandler(SSInstance* instance,
                  TCPRelayHandler* host_tcp_handler,
                  const pp::NetAddress& server_addr,
                  const Crypto::Cipher& cipher,
                  const std::string& password,
                  const int& timeout,
                  const bool& enable_ota,
                  Local& relay_host);
  ~UDPRelayHandler();

  void Sweep();
  void TryLocalRead();
  pp::NetAddress GetBoundAddress();
  void BindServerSocket(pp::CompletionCallback& callback);

 private:
  static const int kBufferSize = 32 * 1024;

  struct NetAddressComp {
    bool operator()(const pp::NetAddress a, const pp::NetAddress b) const {
      if (a.GetFamily() != b.GetFamily()) {
        return a.GetFamily() < b.GetFamily();
      }
      if (a.GetFamily() == PP_NETADDRESS_FAMILY_IPV4) {
        PP_NetAddress_IPv4 netaddrA, netaddrB;
        a.DescribeAsIPv4Address(&netaddrA);
        b.DescribeAsIPv4Address(&netaddrB);
        for (int i = 0; i < 4; ++i) {
          if (netaddrA.addr[i] != netaddrB.addr[i]) {
            return netaddrA.addr[i] < netaddrB.addr[i];
          }
        }
        return netaddrA.port < netaddrB.port;
      } else if (a.GetFamily() == PP_NETADDRESS_FAMILY_IPV6) {
        PP_NetAddress_IPv6 netaddrA, netaddrB;
        a.DescribeAsIPv6Address(&netaddrA);
        b.DescribeAsIPv6Address(&netaddrB);
        for (int i = 0; i < 16; ++i) {
          if (netaddrA.addr[i] != netaddrB.addr[i]) {
            return netaddrA.addr[i] < netaddrB.addr[i];
          }
        }
        return netaddrA.port < netaddrB.port;
      }
      return true;
    }
  };

  SSInstance* instance_;
  pp::UDPSocket server_socket_;
  const pp::NetAddress& server_addr_;
  pp::CompletionCallbackFactory<UDPRelayHandler> callback_factory_;

  Local& relay_host_;
  const int& timeout_;
  const bool& enable_ota_;
  const std::string& password_;
  const Crypto::Cipher& cipher_;
  TCPRelayHandler* const host_tcp_handler_;
  std::vector<uint8_t> uplink_buffer_, downlink_buffer_;
  std::
      map<pp::NetAddress, std::pair<pp::UDPSocket, std::time_t>, NetAddressComp>
          socket_cache_;

  void Sweep(pp::NetAddress local);

  void PerformLocalWrite(pp::NetAddress dest);
  void TryRemoteRead(pp::NetAddress local);
  void PerformRemoteWrite(pp::NetAddress local);
  void PerformRemoteWriteAfterBind(int32_t result, pp::NetAddress local);

  void OnLocalWriteCompletion(int32_t result, pp::NetAddress local);
  void OnLocalReadCompletion(int32_t result, pp::NetAddress source);
  void OnRemoteWriteCompletion(int32_t result, pp::NetAddress local);
  void OnRemoteReadCompletion(int32_t result,
                              pp::NetAddress source,
                              pp::NetAddress from);
};

#endif
