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

#ifndef _SS_SHADOWSOCKS_H_
#define _SS_SHADOWSOCKS_H_

#include <string>
#include <cstdint>
#include "ppapi/cpp/instance.h"

class TCPRelay;

class Shadowsocks {
  public:
    typedef struct {
      std::string server;
      uint16_t server_port;
      std::string method;
      std::string password;
      uint16_t local_port;
      int timetout;  
    } Profile;

    Shadowsocks(pp::Instance *instance)
      : instance_(instance) {}
    ~Shadowsocks();

    void Connect(Profile profile);
    void Sweep();
    void Disconnect();

  private:
    TCPRelay *relay_;
    pp::Instance *instance_;
};

#endif
