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

#include "encrypt.h"

#include <openssl/rand.h>
#include "crypto/openssl.h"
#include "crypto/sodium.h"

Encryptor::Encryptor(const std::string password, const Crypto::Cipher cipher) {
  cipher_info_ = Crypto::GetCipherInfo(cipher);
  key_.resize(cipher_info_->key_size);
  enc_iv_.resize(cipher_info_->iv_size);

  RAND_bytes(enc_iv_.data(), cipher_info_->iv_size);

  const EVP_CIPHER* evp_cipher = nullptr;
  if (cipher_info_->library == Crypto::Library::OPENSSL) {
    evp_cipher = cipher_info_->openssl_cipher();
  } else if (cipher_info_->library == Crypto::Library::SODIUM) {
    // Fake evp cipher for generate 256-bit chacha20/salsa20 key
    evp_cipher = EVP_aes_256_cfb();
  }

  uint8_t temp_iv[32];
  EVP_BytesToKey(evp_cipher, EVP_md5(), nullptr,
                 reinterpret_cast<const unsigned char*>(password.c_str()),
                 password.length(), 1, key_.data(), temp_iv);
}

Encryptor::~Encryptor() {
  delete enc_crypto_;
  delete dec_crypto_;
}

void Encryptor::Encrypt(std::vector<uint8_t>& ciphertext,
                        const std::vector<uint8_t>& plaintext) {
  if (enc_crypto_ == nullptr) {
    if (cipher_info_->library == Crypto::Library::OPENSSL) {
      enc_crypto_ = new CryptoOpenSSL(*cipher_info_, key_, enc_iv_,
                                      Crypto::OpCode::ENCRYPTION);
    } else if (cipher_info_->library == Crypto::Library::SODIUM) {
      enc_crypto_ = new CryptoSodium(*cipher_info_, key_, enc_iv_,
                                     Crypto::OpCode::ENCRYPTION);
    }

    enc_crypto_->Update(ciphertext, plaintext);
    ciphertext.insert(ciphertext.begin(), enc_iv_.begin(), enc_iv_.end());

    return;
  }

  enc_crypto_->Update(ciphertext, plaintext);
}

void Encryptor::Decrypt(std::vector<uint8_t>& plaintext,
                        const std::vector<uint8_t>& ciphertext) {
  if (dec_crypto_ == nullptr) {
    if (ciphertext.size() < cipher_info_->iv_size) {
      return;
    }

    dec_iv_.insert(dec_iv_.begin(), ciphertext.begin(),
                   ciphertext.begin() + cipher_info_->iv_size);
    std::vector<uint8_t> payload(ciphertext.begin() + cipher_info_->iv_size,
                                 ciphertext.end());

    if (cipher_info_->library == Crypto::Library::OPENSSL) {
      dec_crypto_ = new CryptoOpenSSL(*cipher_info_, key_, dec_iv_,
                                      Crypto::OpCode::DECRYPTION);
    } else if (cipher_info_->library == Crypto::Library::SODIUM) {
      dec_crypto_ = new CryptoSodium(*cipher_info_, key_, dec_iv_,
                                     Crypto::OpCode::DECRYPTION);
    }

    dec_crypto_->Update(plaintext, payload);

    return;
  }

  dec_crypto_->Update(plaintext, ciphertext);
}

void Encryptor::UpdateAll(const std::string password,
                          const Crypto::Cipher cipher,
                          std::vector<uint8_t>& out,
                          const std::vector<uint8_t>& in,
                          const Crypto::OpCode enc) {
  auto info = Crypto::GetCipherInfo(cipher);
  std::vector<uint8_t> key(info->key_size), iv(info->iv_size);

  const EVP_CIPHER* evp_cipher = nullptr;
  if (info->library == Crypto::Library::OPENSSL) {
    evp_cipher = info->openssl_cipher();
  } else if (info->library == Crypto::Library::SODIUM) {
    // Fake evp cipher for generate 256-bit chacha20/salsa20 key
    evp_cipher = EVP_aes_256_cfb();
  }

  EVP_BytesToKey(evp_cipher, EVP_md5(), nullptr,
                 reinterpret_cast<const unsigned char*>(password.c_str()),
                 password.length(), 1, key.data(), iv.data());

  std::vector<uint8_t> content;
  std::vector<uint8_t> const* payload = &content;
  if (enc == Crypto::OpCode::ENCRYPTION) {
    payload = &in;
    RAND_bytes(iv.data(), info->iv_size);
  } else if (enc == Crypto::OpCode::DECRYPTION) {
    if (in.size() < info->iv_size) {
      return;
    }
    iv.assign(in.begin(), in.begin() + info->iv_size);
    content.assign(in.begin() + info->iv_size, in.end());
  }

  Crypto* crypto = nullptr;
  if (info->library == Crypto::Library::OPENSSL) {
    crypto = new CryptoOpenSSL(*info, key, iv, enc);
  } else if (info->library == Crypto::Library::SODIUM) {
    crypto = new CryptoSodium(*info, key, iv, enc);
  }

  crypto->Update(out, *payload);

  if (enc == Crypto::OpCode::ENCRYPTION) {
    out.insert(out.begin(), iv.begin(), iv.end());
  }

  delete crypto;
}
