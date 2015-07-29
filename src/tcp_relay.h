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

#ifndef _SS_TCP_RELAY_H_
#define _SS_TCP_RELAY_H_

#include <list>
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/tcp_socket.h"
#include "ppapi/cpp/net_address.h"
#include "ppapi/cpp/host_resolver.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "shadowsocks.h"
#include "tcp_relay_handler.h"

class TCPRelay {
  public:
    TCPRelay(pp::Instance *instance);
    ~TCPRelay();

    void Start(Shadowsocks::Profile profile);
    void Sweep();
    void Sweep(const std::list<TCPRelayHandler*>::iterator &iter);
    void Terminate();

  private:
    static const int kBacklog = 10;

    pp::Instance *instance_;
    pp::HostResolver resolver_;
    pp::NetAddress server_addr_;
    Shadowsocks::Profile profile_;
    Crypto::Cipher const *cipher_;
    pp::TCPSocket listening_socket_;
    std::list<TCPRelayHandler*> handlers_;
    pp::CompletionCallbackFactory<TCPRelay> callback_factory_;

    void OnResolveCompletion(int32_t result);

    void OnBindCompletion(int32_t result);
    void OnListenCompletion(int32_t result);
    void OnAcceptCompletion(int32_t result, pp::TCPSocket socket);
    void OnReadCompletion(int32_t result);
    void OnWriteCompletion(int32_t result);

    void TryAccept();
};

#endif