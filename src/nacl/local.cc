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

#include "local.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctime>
#include <sstream>
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/ppb_console.h"
#include "instance.h"
#include "tcp_relay_handler.h"

Local::Local(SSInstance* instance)
    : instance_(instance), resolver_(instance_), callback_factory_(this) {}

Local::~Local() {
  Terminate();
}

void Local::Start(Shadowsocks::Profile profile) {
  Terminate();

  profile_ = profile;
  cipher_ = Crypto::GetCipher(profile_.method);

  if (cipher_ == nullptr) {
    std::ostringstream status;
    status << "Not a supported encryption method: " << profile_.method;
    instance_->PostStatus(PP_LOGLEVEL_ERROR, status.str());
    return;
  }

  // Resolve server address
  pp::CompletionCallback callback =
      callback_factory_.NewCallback(&Local::OnResolveCompletion);
  PP_HostResolver_Hint hint = {PP_NETADDRESS_FAMILY_UNSPECIFIED, 0};
  resolver_.Resolve(profile_.server.c_str(), profile_.server_port, hint,
                    callback);
}

void Local::Sweep() {
  std::time_t currnet_time = std::time(nullptr);

  for (auto iter = handlers_.begin(); iter != handlers_.end();) {
    if (currnet_time - (*iter)->last_connection_ > profile_.timetout) {
      delete *iter;
      iter = handlers_.erase(iter);
    } else {
      ++iter;
    }
  }
}

void Local::Sweep(const std::list<TCPRelayHandler*>::iterator& iter) {
  delete *iter;
  handlers_.erase(iter);
}

void Local::Terminate() {
  if (!listening_socket_.is_null()) {
    listening_socket_.Close();
  }

  for (auto handler : handlers_) {
    delete handler;
  }
  handlers_.clear();
}

void Local::OnResolveCompletion(int32_t result) {
  if (result != PP_OK) {
    std::ostringstream status;
    status << "Server address resolve Failed with: " << result
           << ". Should be: PP_OK.";
    instance_->PostStatus(PP_LOGLEVEL_ERROR, status.str());
    return;
  }

  server_addr_ = resolver_.GetNetAddress(0);

  listening_socket_ = pp::TCPSocket(instance_);
  PP_NetAddress_IPv4 local = {htons(profile_.local_port), {0}};
  pp::NetAddress addr(instance_, local);
  pp::CompletionCallback callback =
      callback_factory_.NewCallback(&Local::OnBindCompletion);
  int32_t rtn = listening_socket_.Bind(addr, callback);

  if (rtn != PP_OK_COMPLETIONPENDING) {
    std::ostringstream status;
    status << "Error occured when binding server socket: " << result
           << ". Should be: PP_OK_COMPLETIONPENDING.";
    instance_->PostStatus(PP_LOGLEVEL_ERROR, status.str());
    return;
  }
}

void Local::OnBindCompletion(int32_t result) {
  if (result != PP_OK) {
    std::ostringstream status;
    status << "Server Socket Bind Failed with: " << result
           << ". Should be: PP_OK.";
    instance_->PostStatus(PP_LOGLEVEL_ERROR, status.str());
    return;
  }

  pp::CompletionCallback callback =
      callback_factory_.NewCallback(&Local::OnListenCompletion);
  int32_t rtn = listening_socket_.Listen(kBacklog, callback);

  if (rtn != PP_OK_COMPLETIONPENDING) {
    std::ostringstream status;
    status << "Listen Server Socket Failed with: " << result
           << ". Should be: PP_OK_COMPLETIONPENDING.";
    instance_->PostStatus(PP_LOGLEVEL_ERROR, status.str());
    return;
  }
}

void Local::OnListenCompletion(int32_t result) {
  std::ostringstream status;
  if (result != PP_OK) {
    status << "Server Socket Listen Failed with: " << result
           << ". Should be: PP_OK.";
    instance_->PostStatus(PP_LOGLEVEL_ERROR, status.str());
    return;
  }

  status
      << "Listening on: "
      << listening_socket_.GetLocalAddress().DescribeAsString(true).AsString();
  instance_->PostStatus(PP_LOGLEVEL_LOG, status.str());

  TryAccept();
}

void Local::OnAcceptCompletion(int32_t result, pp::TCPSocket socket) {
  if (result != PP_OK) {
    std::ostringstream status;
    status << "Server Socket Accept Failed with: " << result
           << ". Should be: PP_OK.";
    instance_->PostStatus(PP_LOGLEVEL_ERROR, status.str());
    return;
  }

  auto iter = handlers_.insert(
      handlers_.end(), new TCPRelayHandler(instance_, socket, server_addr_,
                                           *cipher_, profile_.password, *this));
  (*iter)->SetHostIter(iter);

  TryAccept();
}

void Local::TryAccept() {
  pp::CompletionCallbackWithOutput<pp::TCPSocket> callback =
      callback_factory_.NewCallbackWithOutput(&Local::OnAcceptCompletion);
  int32_t rtn = listening_socket_.Accept(callback);
  if (rtn != PP_OK_COMPLETIONPENDING) {
    std::ostringstream status;
    status << "Accept Server Socket Failed with: " << rtn
           << ". Should be: PP_OK_COMPLETIONPENDING.";
    instance_->PostStatus(PP_LOGLEVEL_ERROR, status.str());
    return;
  }
}
