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


#include <sstream>
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/var_dictionary.h"
#include "shadowsocks.h"


#ifndef GIT_DESCRIBE
#define GIT_DESCRIBE "unknown"
#endif


class SSInstance : public pp::Instance {
  public:
    explicit SSInstance(PP_Instance instance)
      : pp::Instance(instance),
        socks_(this) {}

    virtual ~SSInstance() {}

    virtual void HandleMessage(const pp::Var& var_message);

  private:
    Shadowsocks socks_;
};


void SSInstance::HandleMessage(const pp::Var& var_message) {

  std::ostringstream status;
  status << "Not a vaild message: ";

  if (!var_message.is_dictionary()) {
    status << "Message should be a dictionary";
    return this->LogToConsole(PP_LOGLEVEL_ERROR, status.str());
  }
  pp::VarDictionary var_dict(var_message);

  if (!var_dict.HasKey(pp::Var("cmd"))) {
    status << "Missing field \"cmd\"";
    return this->LogToConsole(PP_LOGLEVEL_ERROR, status.str());
  }
  pp::Var var_cmd = var_dict.Get(pp::Var("cmd"));

  if (!var_cmd.is_string()) {
    status << "Field \"cmd\" should be a string.";
    return this->LogToConsole(PP_LOGLEVEL_ERROR, status.str());
  }
  std::string cmd = var_cmd.AsString();

  if (cmd == "connect") {   // Need Profile Dictionary as arg
    if (!var_dict.HasKey(pp::Var("arg"))) {
      status << "Command \"connect\" should have field \"arg\"";
      return this->LogToConsole(PP_LOGLEVEL_ERROR, status.str());
    }
    pp::Var var_arg = var_dict.Get(pp::Var("arg"));

    if(!var_arg.is_dictionary()) {
      status << "Field \"arg\" should be a dictionary.";
      return this->LogToConsole(PP_LOGLEVEL_ERROR, status.str());
    }
    pp::VarDictionary dict_arg(var_arg);

    if (!dict_arg.HasKey("server")  || !dict_arg.HasKey("server_port") ||
        !dict_arg.HasKey("method")  || !dict_arg.HasKey("password") ||
        !dict_arg.HasKey("timeout") || !dict_arg.HasKey("local_port")) {
      status << "Not a vaild connect profile, missing required field(s).";
      return this->LogToConsole(PP_LOGLEVEL_ERROR, status.str());
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
      return this->LogToConsole(PP_LOGLEVEL_ERROR, status.str());
    }

    Shadowsocks::Profile profile { 
      server.AsString(),
      static_cast<uint16_t>(server_port.AsInt()),
      method.AsString(),
      password.AsString(),
      static_cast<uint16_t>(local_port.AsInt()),
      timeout.AsInt()
    };
    socks_.Connect(profile);
  } else if (cmd == "sweep") {
    socks_.Sweep();
  } else if (cmd == "disconnect") {
    socks_.Disconnect();
  } else if (cmd == "version") {
    std::ostringstream message;
    message << "Shadowsocks-NaCl Version " << GIT_DESCRIBE;
    this->LogToConsole(PP_LOGLEVEL_LOG, message.str());
  } else {
    status << "cmd \"" << cmd << "\" is not a vaild command.";
    return this->LogToConsole(PP_LOGLEVEL_ERROR, status.str());
  }

}


class SSModule : public pp::Module {
  public:
    SSModule() : pp::Module() {}
    virtual ~SSModule() {}

    virtual pp::Instance* CreateInstance(PP_Instance instance) {
      return new SSInstance(instance);
    }
};


namespace pp {
  Module* CreateModule() { return new SSModule(); }
}
