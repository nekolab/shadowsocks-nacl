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

#include <string>
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/instance.h"
#include "shadowsocks.h"

class SSInstance : public pp::Instance {
  public:
    explicit SSInstance(PP_Instance instance)
      : pp::Instance(instance),
        shadowsocks_(this) {}

    virtual ~SSInstance() {}

    virtual void HandleMessage(const pp::Var &var_message);

    void PostReply(const pp::Var &reply, const pp::Var &msg_id);
    void PostStatus(const PP_LogLevel level, const std::string &status);

  private:
    Shadowsocks shadowsocks_;
};
