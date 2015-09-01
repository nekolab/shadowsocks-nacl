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


#include "instance.h"

#include <sstream>
#include "ppapi/cpp/var_dictionary.h"


void SSInstance::HandleMessage(const pp::Var &var_message) {

  std::ostringstream status;
  status << "Not a vaild message: ";

  if (!var_message.is_dictionary()) {
    status << "Message should be a dictionary";
    return LogToConsole(PP_LOGLEVEL_ERROR, status.str());
  }
  pp::VarDictionary var_dict(var_message);

  if (!var_dict.HasKey(pp::Var("cmd"))) {
    status << "Missing field \"cmd\"";
    return LogToConsole(PP_LOGLEVEL_ERROR, status.str());
  }
  pp::Var var_cmd = var_dict.Get(pp::Var("cmd"));

  if (!var_cmd.is_string()) {
    status << "Field \"cmd\" should be a string.";
    return LogToConsole(PP_LOGLEVEL_ERROR, status.str());
  }
  std::string cmd = var_cmd.AsString();

  if (cmd == "connect") {
  	shadowsocks_.HandleConnectMessage(var_dict);
  } else if (cmd == "sweep") {
    shadowsocks_.HandleSweepMessage(var_dict);
  } else if (cmd == "disconnect") {
    shadowsocks_.HandleDisconnectMessage(var_dict);
  } else if (cmd == "version") {
    shadowsocks_.HandleVersionMessage(var_dict);
  } else if (cmd == "list_methods") {
    shadowsocks_.HandleListMethodsMessage(var_dict);
  } else {
    status << "cmd \"" << cmd << "\" is not a vaild command.";
    return LogToConsole(PP_LOGLEVEL_ERROR, status.str());
  }

}


void SSInstance::PostReply(const pp::Var &reply,
                           const pp::Var &msg_id) {
  pp::VarDictionary message;
  message.Set(pp::Var("type"), pp::Var("reply"));
  message.Set(pp::Var("msg_id"), msg_id);
  message.Set(pp::Var("payload"), reply);
  PostMessage(message);
}


void SSInstance::PostStatus(const PP_LogLevel level,
                            const std::string &status) {
  pp::VarDictionary message;
  message.Set(pp::Var("type"), pp::Var("status"));
  message.Set(pp::Var("message"), pp::Var(status));

  switch(level) {
    case PP_LOGLEVEL_TIP:
      message.Set(pp::Var("level"), pp::Var("success")); break;
    case PP_LOGLEVEL_LOG:
      message.Set(pp::Var("level"), pp::Var("info")); break;
    case PP_LOGLEVEL_WARNING:
      message.Set(pp::Var("level"), pp::Var("warning")); break;
    case PP_LOGLEVEL_ERROR:
      message.Set(pp::Var("level"), pp::Var("danger")); break;
  }

  LogToConsole(level, status);
  PostMessage(message);
}
