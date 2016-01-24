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

#ifndef _SS_SOCKS5_H_
#define _SS_SOCKS5_H_

#include <cstdint>
#include <string>
#include <vector>
#include "ppapi/cpp/net_address.h"

struct Socks5 {
  static const uint8_t VER = 0x05;
  static const uint8_t RSV = 0x00;

  enum Stage {
    WAIT_AUTH,
    AUTH_OK,
    AUTH_FAIL,
    WAIT_CMD,
    CMD_CONNECT,
    CMD_BIND,
    CMD_UDP_ASSOC,
    TCP_RELAY,
    UDP_RELAY
  };

  enum Auth {
    NO_AUTH = 0x00,
    GSSAPI = 0x01,
    PASSWORD = 0x02,
    NO_ACCEPTABLE = 0xff
  };

  enum Cmd { CONNECT = 0x01, BIND = 0x02, UDP_ASSOC = 0x03 };

  enum Atyp { IPv4 = 0x01, DOMAINNAME = 0x03, IPv6 = 0x04 };

  enum Rep {
    SUCCEEDED = 0x00,
    GENERAL_FAILURE = 0x01,
    CONNECT_NOT_ALLOWED = 0x02,
    NETWORK_UNREACHABLE = 0x03,
    HOST_UNREACHABLE = 0x04,
    CONNECTION_REFUSED = 0x05,
    TTL_EXPIRED = 0x06,
    COMMAND_NOT_SUPPORTED = 0x07,
    ADDR_TYPE_NOT_SUPPORTED = 0x08
  };

  typedef struct ConsultPacket {
    uint8_t VER = 0x05;
    union {
      uint8_t CMD;
      uint8_t REP;
    };
    uint8_t RSV = 0x00;
    uint8_t ATYP;
    pp::NetAddress IP;
    struct {
      std::string HOST;
      uint16_t PORT;
    } DOMAIN;
  } ConsultPacket;

  static int ParseHeader(ConsultPacket& request,
                         const std::vector<uint8_t>& header);

  static int PackResponse(std::vector<uint8_t>& resp,
                          const ConsultPacket& reply);
};

#endif
