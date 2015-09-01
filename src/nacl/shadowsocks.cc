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


#include "shadowsocks.h"

#include <sstream>
#include "ppapi/cpp/var.h"
#include "instance.h"
#include "tcp_relay.h"
#include "crypto/crypto.h"


#ifndef GIT_DESCRIBE
#define GIT_DESCRIBE "unknown"
#endif


Shadowsocks::~Shadowsocks() {
  delete relay_;
}


void Shadowsocks::Connect(Profile profile) {
  if (relay_ != nullptr) {
    Disconnect();
  }

  relay_ = new TCPRelay(instance_);
  relay_->Start(profile);
}


void Shadowsocks::Sweep() {
  if (relay_ != nullptr) {
  	relay_->Sweep();
  }
}


void Shadowsocks::Disconnect() {
  delete relay_;
  relay_ = nullptr;
}


void Shadowsocks::HandleConnectMessage(const pp::VarDictionary &var_dict) {

  std::ostringstream status;
  status << "Not a vaild message: ";

  if (!var_dict.HasKey(pp::Var("arg"))) {
    status << "Command \"connect\" should have field \"arg\"";
    return instance_->LogToConsole(PP_LOGLEVEL_ERROR, status.str());
  }
  pp::Var var_arg = var_dict.Get(pp::Var("arg"));

  if(!var_arg.is_dictionary()) {
    status << "Field \"arg\" should be a dictionary.";
    return instance_->LogToConsole(PP_LOGLEVEL_ERROR, status.str());
  }
  pp::VarDictionary dict_arg(var_arg);

  if (!dict_arg.HasKey("server")  || !dict_arg.HasKey("server_port") ||
      !dict_arg.HasKey("method")  || !dict_arg.HasKey("password") ||
      !dict_arg.HasKey("timeout") || !dict_arg.HasKey("local_port")) {
    status << "Not a vaild connect profile, missing required field(s).";
    return instance_->LogToConsole(PP_LOGLEVEL_ERROR, status.str());
  }

  pp::Var method = dict_arg.Get("method"),
          server = dict_arg.Get("server"),
          timeout = dict_arg.Get("timeout"),
          password = dict_arg.Get("password"),
          local_port = dict_arg.Get("local_port"),
          server_port = dict_arg.Get("server_port");

  if (!method.is_string() || !server.is_string() ||
      !timeout.is_int() || !password.is_string() ||
      !local_port.is_int() || !server_port.is_int()) {
    status << "Not a vaild connect profile, field type error.";
    return instance_->LogToConsole(PP_LOGLEVEL_ERROR, status.str());
  }

  Shadowsocks::Profile profile { 
    server.AsString(),
    static_cast<uint16_t>(server_port.AsInt()),
    method.AsString(),
    password.AsString(),
    static_cast<uint16_t>(local_port.AsInt()),
    timeout.AsInt()
  };
  
  Connect(profile);

  if (var_dict.HasKey("msg_id")) {
    instance_->PostReply(pp::Var(PP_OK), var_dict.Get("msg_id"));
  }
}


void Shadowsocks::HandleSweepMessage(const pp::VarDictionary &var_dict) {
  Sweep();
  if (var_dict.HasKey("msg_id")) {
    instance_->PostReply(pp::Var(PP_OK), var_dict.Get("msg_id"));
  }
}


void Shadowsocks::HandleDisconnectMessage(const pp::VarDictionary &var_dict) {
  Disconnect();
  if (var_dict.HasKey("msg_id")) {
    instance_->PostReply(pp::Var(PP_OK), var_dict.Get("msg_id"));
  }
}


void Shadowsocks::HandleVersionMessage(const pp::VarDictionary &var_dict) {
  if (var_dict.HasKey("msg_id")) {
    pp::VarDictionary reply;
    reply.Set(pp::Var("version"), pp::Var(GIT_DESCRIBE));
    instance_->PostReply(reply, var_dict.Get("msg_id"));
  } else {
    std::ostringstream message;
    message << "Shadowsocks-NaCl Version " << GIT_DESCRIBE;
    instance_->LogToConsole(PP_LOGLEVEL_LOG, message.str());
  }
}


void Shadowsocks::HandleListMethodsMessage(const pp::VarDictionary &var_dict) {
  if (var_dict.HasKey("msg_id")) {
    auto list = Crypto::GetSupportedCipherNames();
    pp::VarArray reply;
    for (auto method : list) {
      reply.Set(reply.GetLength(), pp::Var(method));
    }
    instance_->PostReply(reply, var_dict.Get("msg_id"));
  }
}
