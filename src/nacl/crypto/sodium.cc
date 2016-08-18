/*
 * Copyright (C) 2016  Sunny <ratsunny@gmail.com>
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

bool CryptoSodium::Update(std::vector<uint8_t>* out,
                          const std::vector<uint8_t>& in) {
  auto in_len = in.size();
  int padding = counter_ % BLOCK_SIZE;

  if (out->size() < padding + in_len) {
    out->resize((padding + in_len) * 2);
  }

  std::vector<uint8_t> content;
  const std::vector<uint8_t>* payload = &in;
  if (padding) {
    content.insert(content.end(), padding, 0);
    content.insert(content.end(), in.begin(), in.begin() + in_len);
    payload = &content;
  }

  if (cipher_info_.sodium_cipher(out->data(), payload->data(), padding + in_len,
                                 iv_.data(), counter_ / BLOCK_SIZE,
                                 key_.data())) {
    return false;
  }
  counter_ += in_len;

  out->erase(out->begin(), out->begin() + padding);
  out->resize(in_len);
  return true;
}
