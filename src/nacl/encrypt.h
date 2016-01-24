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

#ifndef _SS_ENCRYPT_H_
#define _SS_ENCRYPT_H_

#include "crypto/crypto.h"

class Encryptor {
 public:
  Encryptor(const std::string password, const Crypto::Cipher cipher);
  ~Encryptor();

  void Encrypt(std::vector<uint8_t>& ciphertext,
               const std::vector<uint8_t>& plaintext);
  void Decrypt(std::vector<uint8_t>& plaintext,
               const std::vector<uint8_t>& ciphertext);

  static void UpdateAll(const std::string password,
                        const Crypto::Cipher cipher,
                        std::vector<uint8_t>& out,
                        const std::vector<uint8_t>& in,
                        const Crypto::OpCode enc);

 private:
  Crypto *enc_crypto_ = nullptr, *dec_crypto_ = nullptr;
  const Crypto::CipherInfo* cipher_info_;
  std::vector<uint8_t> key_, enc_iv_, dec_iv_;
};

#endif
