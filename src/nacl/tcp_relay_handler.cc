/*
 * Copyright (C) 2015  Sunny <ratsunny@gmail.com>
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


#include "tcp_relay_handler.h"

#include <algorithm>
#include <sstream>
#include "ppapi/c/ppb_console.h"
#include "local.h"
#include "instance.h"


TCPRelayHandler::TCPRelayHandler(SSInstance *instance,
                                 pp::TCPSocket socket,
                                 const pp::NetAddress &server_addr,
                                 const Crypto::Cipher &cipher,
                                 const std::string &password,
                                 Local &relay_host)
  : instance_(instance),
    local_socket_(socket),
    remote_socket_(instance),
    server_addr_(server_addr),
    callback_factory_(this),
    encryptor_(password, cipher),
    stage_(Socks5::Stage::WAIT_AUTH),
    uplink_buffer_(kBufferSize, 0),
    downlink_buffer_(kBufferSize, 0),
    relay_host_(relay_host) {

  uplink_buffer_.reserve(kBufferSize);
  downlink_buffer_.reserve(kBufferSize);

  TryLocalRead();
}


TCPRelayHandler::~TCPRelayHandler() {
  local_socket_.Close();
  remote_socket_.Close();
}


void TCPRelayHandler::SetHostIter(const std::list<TCPRelayHandler*>::iterator host_iter) {
  host_iter_ = host_iter;
}


void TCPRelayHandler::OnRemoteReadCompletion(int32_t result) {
  if (result < 0) {
    return relay_host_.Sweep(host_iter_);
  }

  last_connection_ = std::time(nullptr);
  downlink_buffer_.resize(result);

  switch (stage_) {
    case Socks5::Stage::CMD_CONNECT:
    case Socks5::Stage::TCP_RELAY: {
      std::vector<uint8_t> out;
      encryptor_.Decrypt(out, downlink_buffer_);
      std::swap(out, downlink_buffer_);
      PerformLocalWrite();
    } break;
    case Socks5::Stage::UDP_RELAY:
      break;
    default:
      return relay_host_.Sweep(host_iter_);
  }
}


void TCPRelayHandler::OnRemoteWriteCompletion(int32_t result) {
  if (result < 0) {
    return relay_host_.Sweep(host_iter_);
  }

  last_connection_ = std::time(nullptr);

  if (result < uplink_buffer_.size()) {
    instance_->LogToConsole(PP_LOGLEVEL_TIP, "Not a full remote write");
    uplink_buffer_.erase(uplink_buffer_.begin(),
                         uplink_buffer_.begin() + result);
    return PerformRemoteWrite();
  }

  switch (stage_) {
    case Socks5::Stage::CMD_CONNECT:
      downlink_buffer_.clear();
      downlink_buffer_.push_back(Socks5::VER);
      downlink_buffer_.push_back(Socks5::Rep::SUCCEEDED);
      downlink_buffer_.push_back(Socks5::RSV);
      downlink_buffer_.push_back(Socks5::Atyp::IPv4);
      downlink_buffer_.resize(10, 0); // Fill IP and Port with 0
      PerformLocalWrite();
      break;
    case Socks5::Stage::TCP_RELAY:
      TryLocalRead();
      break;
    case Socks5::Stage::UDP_RELAY:
      break;
    default:
      return relay_host_.Sweep(host_iter_);
  }
}


void TCPRelayHandler::OnLocalReadCompletion(int32_t result) {
  if (result < 0) {
    return relay_host_.Sweep(host_iter_);
  }

  last_connection_ = std::time(nullptr);
  uplink_buffer_.resize(result);

  switch (stage_) {
    case Socks5::Stage::WAIT_AUTH:
      HandleAuth();
      break;
    case Socks5::Stage::WAIT_CMD:
      HandleCommand();
      break;
    case Socks5::Stage::TCP_RELAY: {
      std::vector<uint8_t> out;
      encryptor_.Encrypt(out, uplink_buffer_);
      std::swap(out, uplink_buffer_);
      PerformRemoteWrite();
    } break;
    case Socks5::Stage::UDP_RELAY:
      break;
    default:
      return relay_host_.Sweep(host_iter_);
  }
}


void TCPRelayHandler::OnLocalWriteCompletion(int32_t result) {
  if (result < 0) {
    return relay_host_.Sweep(host_iter_);
  }

  last_connection_ = std::time(nullptr);

  if (result < downlink_buffer_.size()) {
    instance_->LogToConsole(PP_LOGLEVEL_TIP, "Not a full local write");
    downlink_buffer_.erase(downlink_buffer_.begin(),
                           downlink_buffer_.begin() + result);
    return PerformLocalWrite();
  }

  switch (stage_) {
    case Socks5::Stage::AUTH_OK:
      stage_ = Socks5::Stage::WAIT_CMD;
      TryLocalRead();
      break;
    case Socks5::Stage::CMD_CONNECT:
      stage_ = Socks5::Stage::TCP_RELAY;
      TryLocalRead();
      TryRemoteRead();
      break;
    case Socks5::Stage::CMD_UDP_ASSOC:
      stage_ = Socks5::Stage::UDP_RELAY;
      break;
    case Socks5::Stage::TCP_RELAY:
      TryRemoteRead();
      break;
    default:
      return relay_host_.Sweep(host_iter_);
  }
}


void TCPRelayHandler::HandleAuth() {
  if (uplink_buffer_[0] != Socks5::VER) {
    return relay_host_.Sweep(host_iter_);
  }

  downlink_buffer_.clear();
  downlink_buffer_.push_back(Socks5::VER);
  if (uplink_buffer_.end() != std::find(std::begin(uplink_buffer_) + 2,
                                        std::end(uplink_buffer_),
                                        Socks5::Auth::NO_AUTH)) {
    stage_ = Socks5::Stage::AUTH_OK;
    downlink_buffer_.push_back(Socks5::Auth::NO_AUTH);
  } else {
    stage_ = Socks5::Stage::AUTH_FAIL;
    downlink_buffer_.push_back(Socks5::Auth::NO_ACCEPTABLE);
  }

  PerformLocalWrite();
}


void TCPRelayHandler::HandleCommand() {
  if (Socks5::ParseHeader(packet_, uplink_buffer_) == 0) {
    return relay_host_.Sweep(host_iter_);
  }

  switch (packet_.CMD) {
    case Socks5::Cmd::CONNECT: {
      stage_ = Socks5::Stage::CMD_CONNECT;
      pp::CompletionCallback callback =
          callback_factory_.NewCallback(&TCPRelayHandler::HandleConnectCmd);
      int32_t rtn = remote_socket_.Connect(server_addr_, callback);
      if (rtn != PP_OK_COMPLETIONPENDING) {
        std::ostringstream status;
        status << "Connect to server failed: " << rtn
               << ". Should be: PP_OK_COMPLETIONPENDING.";
        instance_->PostStatus(PP_LOGLEVEL_ERROR, status.str());
        return relay_host_.Sweep(host_iter_);
      }
    } break;
    case Socks5::Cmd::BIND:
      stage_ = Socks5::Stage::CMD_BIND;
      downlink_buffer_.clear();
      downlink_buffer_.push_back(Socks5::VER);
      downlink_buffer_.push_back(Socks5::Rep::COMMAND_NOT_SUPPORTED);
      downlink_buffer_.push_back(Socks5::RSV);
      downlink_buffer_.push_back(Socks5::Atyp::IPv4);
      downlink_buffer_.resize(10, 0);
      PerformLocalWrite();
      break;
    case Socks5::Cmd::UDP_ASSOC:
      stage_ = Socks5::Stage::CMD_UDP_ASSOC;
      HandleUDPAssocCmd(0);
      break;
  }
}


void TCPRelayHandler::HandleConnectCmd(int32_t result) {
  if (result != PP_OK) {
    std::ostringstream status;
    status << "Failed to connect to server: "
           << result << ". Should be: PP_OK";
    instance_->PostStatus(PP_LOGLEVEL_LOG, status.str());
    return relay_host_.Sweep(host_iter_);
  }

  uplink_buffer_.erase(uplink_buffer_.begin(), uplink_buffer_.begin() + 3);
  std::vector<uint8_t> out;
  encryptor_.Encrypt(out, uplink_buffer_);
  std::swap(out, uplink_buffer_);
  PerformRemoteWrite();
}


void TCPRelayHandler::HandleUDPAssocCmd(int32_t result) {

}


void TCPRelayHandler::TryLocalRead() {
  uplink_buffer_.resize(kBufferSize);
  pp::CompletionCallback callback =
      callback_factory_.NewCallback(&TCPRelayHandler::OnLocalReadCompletion);
  int32_t rtn = local_socket_.Read(
      (char*)uplink_buffer_.data(), kBufferSize, callback);
  if (rtn != PP_OK_COMPLETIONPENDING) {
    return relay_host_.Sweep(host_iter_);
  }
}


void TCPRelayHandler::TryRemoteRead() {
  downlink_buffer_.resize(kBufferSize);
  pp::CompletionCallback callback =
      callback_factory_.NewCallback(&TCPRelayHandler::OnRemoteReadCompletion);
  int32_t rtn = remote_socket_.Read(
      (char*)downlink_buffer_.data(), kBufferSize, callback);
  if (rtn != PP_OK_COMPLETIONPENDING) {
    return relay_host_.Sweep(host_iter_);
  }
}


void TCPRelayHandler::PerformLocalWrite() {
  pp::CompletionCallback callback =
      callback_factory_.NewCallback(&TCPRelayHandler::OnLocalWriteCompletion);
  int32_t rtn = local_socket_.Write(
      (char*)downlink_buffer_.data(), downlink_buffer_.size(), callback);
  if (rtn != PP_OK_COMPLETIONPENDING) {
    return relay_host_.Sweep(host_iter_);
  }
}


void TCPRelayHandler::PerformRemoteWrite() {
  pp::CompletionCallback callback =
      callback_factory_.NewCallback(&TCPRelayHandler::OnRemoteWriteCompletion);
  int32_t rtn = remote_socket_.Write(
      (char*)uplink_buffer_.data(), uplink_buffer_.size(), callback);
  if (rtn != PP_OK_COMPLETIONPENDING) {
    return relay_host_.Sweep(host_iter_);
  }
}
