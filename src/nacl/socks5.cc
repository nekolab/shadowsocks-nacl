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

#include "socks5.h"

#include "ppapi/c/ppb_net_address.h"

const uint8_t Socks5::VER, Socks5::RSV;

int Socks5::ParseHeader(ConsultPacket& request,
                        const std::vector<uint8_t>& header) {
  if (header[0] != VER || header[2] != RSV) {
    return 0;
  }

  request.CMD = header[1];
  request.ATYP = header[3];

  switch (request.ATYP) {
    case IPv4:
      if (header.size() < 10) {
        return 0;
      }
      return 10;
    case IPv6:
      if (header.size() < 22) {
        return 0;
      }
      return 22;
    case DOMAINNAME:
      if (header.size() < (5 + header[4] + 2)) {
        return 0;
      }
      return 5 + header[4] + 2;
  }

  return 0;
}

int Socks5::PackResponse(std::vector<uint8_t>& resp,
                         const ConsultPacket& reply) {
  resp.clear();
  resp.push_back(reply.VER);
  resp.push_back(reply.REP);
  resp.push_back(reply.RSV);
  resp.push_back(reply.ATYP);

  switch (reply.ATYP) {
    case IPv4: {
      PP_NetAddress_IPv4 ipv4_addr;
      reply.IP.DescribeAsIPv4Address(&ipv4_addr);
      for (uint8_t c : ipv4_addr.addr) {
        resp.push_back(c);
      }
      resp.push_back(ipv4_addr.port >> 8);
      resp.push_back(ipv4_addr.port & 0xff);
      return 1;
    }
    case IPv6: {
      PP_NetAddress_IPv6 ipv6_addr;
      reply.IP.DescribeAsIPv6Address(&ipv6_addr);
      for (uint8_t c : ipv6_addr.addr) {
        resp.push_back(c);
      }
      resp.push_back(ipv6_addr.port >> 8);
      resp.push_back(ipv6_addr.port & 0xff);
      return 1;
    }
    case DOMAINNAME: {
      std::string::size_type len = reply.DOMAIN.HOST.length();
      const char* str = reply.DOMAIN.HOST.c_str();
      resp.push_back(len);
      for (decltype(len) i = 0; i < len; ++i) {
        resp.push_back(*(str + i));
      }
      resp.push_back(reply.DOMAIN.PORT >> 8);
      resp.push_back(reply.DOMAIN.PORT & 0xff);
      return 1;
    }
  }

  return 0;
}
