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

#include "udp_relay_handler.h"

#include <sstream>
#include "ppapi/c/ppb_console.h"
#include "local.h"
#include "instance.h"
#include "tcp_relay_handler.h"

UDPRelayHandler::UDPRelayHandler(SSInstance* instance,
                                 TCPRelayHandler* host_tcp_handler,
                                 const pp::NetAddress& server_addr,
                                 const Crypto::Cipher& cipher,
                                 const std::string& password,
                                 const int& timeout,
                                 Local& relay_host)
    : instance_(instance),
      server_socket_(instance),
      server_addr_(server_addr),
      callback_factory_(this),
      relay_host_(relay_host),
      timeout_(timeout),
      password_(password),
      cipher_(cipher),
      host_tcp_handler_(host_tcp_handler),
      uplink_buffer_(kBufferSize, 0),
      downlink_buffer_(kBufferSize, 0) {}

UDPRelayHandler::~UDPRelayHandler() {
  server_socket_.Close();
  for (auto socket_pair : socket_cache_) {
    socket_pair.second.first.Close();
  }
  socket_cache_.clear();
}

void UDPRelayHandler::Sweep() {
  std::time_t current_time = std::time(nullptr);

  for (auto iter = socket_cache_.begin(); iter != socket_cache_.end();) {
    if (current_time - iter->second.second > timeout_) {
      iter->second.first.Close();
      iter = socket_cache_.erase(iter);
    } else {
      ++iter;
    }
  }
}

void UDPRelayHandler::Sweep(pp::NetAddress local) {
  auto remote_socket_pair_iter = socket_cache_.find(local);
  if (remote_socket_pair_iter == socket_cache_.end()) {
    return;
  }
  remote_socket_pair_iter->second.first.Close();
  socket_cache_.erase(remote_socket_pair_iter);
}

pp::NetAddress UDPRelayHandler::GetBoundAddress() {
  return server_socket_.GetBoundAddress();
}

void UDPRelayHandler::BindServerSocket(pp::CompletionCallback& callback) {
  PP_NetAddress_IPv4 addr = {0, {127, 0, 0, 1}};
  pp::NetAddress bind_addr(instance_, addr);
  server_socket_.Bind(bind_addr, callback);
}

void UDPRelayHandler::OnLocalWriteCompletion(int32_t result,
                                             pp::NetAddress local) {
  if (result < 0) {
    std::ostringstream status;
    status << "Failed write to local UDP socket: " << result;
    instance_->PostStatus(PP_LOGLEVEL_LOG, status.str());
    return Sweep(local);
  }

  TryRemoteRead(local);
}

void UDPRelayHandler::OnRemoteWriteCompletion(int32_t result,
                                              pp::NetAddress local) {
  if (result < 0) {
    std::ostringstream status;
    status << "Failed write to remote UDP socket: " << result;
    instance_->PostStatus(PP_LOGLEVEL_LOG, status.str());
    return Sweep(local);
  }

  TryLocalRead();
}

void UDPRelayHandler::OnLocalReadCompletion(int32_t result,
                                            pp::NetAddress source) {
  if (result < 0) {
    std::ostringstream status;
    status << "Failed to receive from local UDP socket: " << result;
    instance_->PostStatus(PP_LOGLEVEL_LOG, status.str());
    return relay_host_.Sweep(host_tcp_handler_->host_iter_);
  }

  if (result < 3 || uplink_buffer_[2] != 0x00) {
    return TryLocalRead();
  }

  uplink_buffer_.resize(result);
  uplink_buffer_.erase(uplink_buffer_.begin(), uplink_buffer_.begin() + 3);
  Encryptor::UpdateAll(password_, cipher_, &uplink_buffer_, uplink_buffer_,
                       Crypto::OpCode::ENCRYPTION);

  auto remote_socket_pair_iter = socket_cache_.find(source);
  if (remote_socket_pair_iter == socket_cache_.end()) {
    pp::UDPSocket remote_socket(instance_);
    std::time_t last_connection = std::time(nullptr);
    socket_cache_[source] = std::make_pair(remote_socket, last_connection);
    PP_NetAddress_IPv4 addr = {0, {127, 0, 0, 1}};
    pp::NetAddress bind_addr(instance_, addr);
    auto callback = callback_factory_.NewCallback(
        &UDPRelayHandler::PerformRemoteWriteAfterBind, source);
    remote_socket.Bind(bind_addr, callback);
  } else {
    PerformRemoteWrite(source);
  }
}

void UDPRelayHandler::OnRemoteReadCompletion(int32_t result,
                                             pp::NetAddress source,
                                             pp::NetAddress local) {
  if (result < 0) {
    std::ostringstream status;
    status << "Failed to read UDP from remote socket: " << result;
    instance_->PostStatus(PP_LOGLEVEL_LOG, status.str());
    return Sweep(local);
  }

  downlink_buffer_.resize(result);
  Encryptor::UpdateAll(password_, cipher_, &downlink_buffer_, downlink_buffer_,
                       Crypto::OpCode::DECRYPTION);
  downlink_buffer_.insert(downlink_buffer_.begin(), 3, 0);
  PerformLocalWrite(local);
}

void UDPRelayHandler::TryLocalRead() {
  uplink_buffer_.resize(kBufferSize);
  auto callback = callback_factory_.NewCallbackWithOutput(
      &UDPRelayHandler::OnLocalReadCompletion);
  int32_t rtn = server_socket_.RecvFrom((char*)uplink_buffer_.data(),
                                        kBufferSize, callback);
  if (rtn != PP_OK_COMPLETIONPENDING) {
    return relay_host_.Sweep(host_tcp_handler_->host_iter_);
  }
}

void UDPRelayHandler::TryRemoteRead(pp::NetAddress local) {
  downlink_buffer_.resize(kBufferSize);
  pp::UDPSocket remote_socket = socket_cache_[local].first;
  auto callback = callback_factory_.NewCallbackWithOutput(
      &UDPRelayHandler::OnRemoteReadCompletion, local);
  int32_t rtn = remote_socket.RecvFrom((char*)downlink_buffer_.data(),
                                       kBufferSize, callback);
  if (rtn != PP_OK_COMPLETIONPENDING) {
    return Sweep(local);
  }
}

void UDPRelayHandler::PerformLocalWrite(pp::NetAddress dest) {
  auto remote_socket_pair_iter = socket_cache_.find(dest);
  if (remote_socket_pair_iter == socket_cache_.end()) {
    return relay_host_.Sweep(host_tcp_handler_->host_iter_);
  }
  std::time(&host_tcp_handler_->last_connection_);
  std::time(&remote_socket_pair_iter->second.second);

  pp::CompletionCallback callback = callback_factory_.NewCallback(
      &UDPRelayHandler::OnLocalWriteCompletion, dest);
  int32_t rtn = server_socket_.SendTo((char*)downlink_buffer_.data(),
                                      downlink_buffer_.size(), dest, callback);
  if (rtn != PP_OK_COMPLETIONPENDING) {
    return relay_host_.Sweep(host_tcp_handler_->host_iter_);
  }
}

void UDPRelayHandler::PerformRemoteWrite(pp::NetAddress local) {
  auto remote_socket_pair_iter = socket_cache_.find(local);
  if (remote_socket_pair_iter == socket_cache_.end()) {
    return relay_host_.Sweep(host_tcp_handler_->host_iter_);
  }

  auto remote_socket_pair = remote_socket_pair_iter->second;
  std::time(&remote_socket_pair.second);
  std::time(&host_tcp_handler_->last_connection_);
  pp::UDPSocket socket = remote_socket_pair.first;

  pp::CompletionCallback callback = callback_factory_.NewCallback(
      &UDPRelayHandler::OnRemoteWriteCompletion, local);
  int32_t rtn = socket.SendTo((char*)uplink_buffer_.data(),
                              uplink_buffer_.size(), server_addr_, callback);
  if (rtn != PP_OK_COMPLETIONPENDING) {
    return Sweep(local);
  }
}

void UDPRelayHandler::PerformRemoteWriteAfterBind(int32_t result,
                                                  pp::NetAddress local) {
  if (result != PP_OK) {
    std::ostringstream status;
    status << "Failed to perform remote UDP socket bind: " << result
           << ". Should be: PP_OK";
    instance_->PostStatus(PP_LOGLEVEL_LOG, status.str());
    return Sweep(local);
  }
  PerformRemoteWrite(local);
  TryRemoteRead(local);
}
