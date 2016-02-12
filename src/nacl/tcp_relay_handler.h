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

#ifndef _SS_TCP_RELAY_HANDLER_H_
#define _SS_TCP_RELAY_HANDLER_H_

#include <list>
#include <ctime>
#include "ppapi/cpp/tcp_socket.h"
#include "ppapi/cpp/net_address.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "socks5.h"
#include "encrypt.h"

class Local;
class SSInstance;
class UDPRelayHandler;

class TCPRelayHandler {
 public:
  friend class UDPRelayHandler;

  TCPRelayHandler(SSInstance* instance,
                  pp::TCPSocket socket,
                  const pp::NetAddress& server_addr,
                  const Crypto::Cipher& cipher,
                  const std::string& password,
                  const int& timeout,
                  Local& relay_host);
  ~TCPRelayHandler();

  std::time_t last_connection_;

  void SweepUDP();  // Sweep unused UDP server port if exists
  void SetHostIter(const std::list<TCPRelayHandler*>::iterator host_iter);

 private:
  static const int kBufferSize = 32 * 1024;

  SSInstance* instance_;
  pp::TCPSocket local_socket_;
  pp::TCPSocket remote_socket_;
  const pp::NetAddress& server_addr_;
  pp::CompletionCallbackFactory<TCPRelayHandler> callback_factory_;

  Local& relay_host_;
  const int& timeout_;
  Encryptor encryptor_;
  Socks5::Stage stage_;
  const std::string& password_;
  const Crypto::Cipher& cipher_;
  UDPRelayHandler* udp_relay_handler_;
  std::list<TCPRelayHandler*>::iterator host_iter_;
  std::vector<uint8_t> uplink_buffer_, downlink_buffer_;

  void OnRemoteReadCompletion(int32_t result);
  void OnRemoteWriteCompletion(int32_t result);
  void OnLocalReadCompletion(int32_t result);
  void OnLocalWriteCompletion(int32_t result);

  void HandleAuth();
  void HandleCommand();
  void HandleConnectCmd(int32_t result);
  void HandleUDPAssocCmd(int32_t result);

  void TryLocalRead();
  void TryRemoteRead();
  void PerformLocalWrite();
  void PerformRemoteWrite();
};

#endif
