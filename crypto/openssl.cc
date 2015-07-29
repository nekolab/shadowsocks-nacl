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


#include "openssl.h"

#include <openssl/md5.h>


CryptoOpenSSL::CryptoOpenSSL( const Crypto::CipherInfo &cipher_info,
                              const std::vector<uint8_t> key,
                              const std::vector<uint8_t> iv,
                              const Crypto::OpCode enc)
  : cipher_info_(cipher_info), key_(key), iv_(iv), enc_(enc) {

    EVP_CIPHER_CTX_init(&ctx_);

    if (cipher_info_.openssl_cipher == &EVP_rc4) {
      std::vector<uint8_t> key_iv;
      key_iv.insert(key_iv.end(), key_.begin(), key_.end());
      key_iv.insert(key_iv.end(), iv_.begin(), iv_.end());

      EVP_CipherInit_ex(&ctx_, (cipher_info_.openssl_cipher)(), nullptr,
        MD5(key_iv.data(), 32, nullptr), iv_.data(), static_cast<int>(enc_));
    } else {
      EVP_CipherInit_ex(&ctx_, (cipher_info_.openssl_cipher)(),
        nullptr, key_.data(), iv_.data(), static_cast<int>(enc_));
    }

}


CryptoOpenSSL::~CryptoOpenSSL() {
  EVP_CIPHER_CTX_cleanup(&ctx_);
}


void CryptoOpenSSL::Update(std::vector<uint8_t> &out,
                           const std::vector<uint8_t> &in) {

  int ilen = in.size(), olen = out.size();

  if (olen < ilen + 31) {
    out.resize(ilen + 31);
  }

  EVP_CipherUpdate(&ctx_, out.data(), &olen, in.data(), ilen);
  out.resize(olen);

}
