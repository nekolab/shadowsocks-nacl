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


#include "sodium.h"

#include "nacl_io/nacl_io.h"


CryptoSodium::CryptoSodium(const Crypto::CipherInfo &cipher_info,
                           const std::vector<uint8_t> key,
                           const std::vector<uint8_t> iv,
                           const Crypto::OpCode enc)
  : cipher_info_(cipher_info), key_(key), iv_(iv), enc_(enc), counter_(0) {

  nacl_io_init();
  sodium_init();
}


void CryptoSodium::Update(std::vector<uint8_t> &out,
                          const std::vector<uint8_t> &in) {

  auto in_len = in.size();
  int padding = counter_ % BLOCK_SIZE;

  if (out.size() < padding + in_len) {
    out.resize((padding + in_len) * 2);
  }

  std::vector<uint8_t> content;
  std::vector<uint8_t> const *payload = &in;
  if (padding) {
    content.insert(content.end(), padding, 0);
    content.insert(content.end(), in.begin(), in.end());
    payload = &content;
  }

  cipher_info_.sodium_cipher(out.data(), payload->data(),
                             payload->size(),
                             iv_.data(), counter_ / BLOCK_SIZE,
                             key_.data());
  counter_ += in_len;

  out.erase(out.begin(), out.begin() + padding);
  out.resize(in_len);
}
