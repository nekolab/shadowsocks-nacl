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

#ifndef _SS_SODIUM_H_
#define _SS_SODIUM_H_

#include "crypto.h"

class CryptoSodium : public Crypto {
  public:
    static bool initialized_;

    static const int BLOCK_SIZE = 64;

    const Crypto::CipherInfo &cipher_info_;
    const std::vector<uint8_t> key_;
    const std::vector<uint8_t> iv_;
    const Crypto::OpCode enc_;

    CryptoSodium(const Crypto::CipherInfo &cipher_info,
                 const std::vector<uint8_t> key,
                 const std::vector<uint8_t> iv,
                 const Crypto::OpCode enc);
    ~CryptoSodium() {}

    void Update(std::vector<uint8_t> &out,
                const std::vector<uint8_t> &in) override;

  private:
    uint64_t counter_;
};

#endif
