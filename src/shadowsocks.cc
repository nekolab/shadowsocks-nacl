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

#include "tcp_relay.h"


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
