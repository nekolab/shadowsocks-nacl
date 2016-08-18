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

#include "encrypt.h"

#include <netinet/in.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include "crypto/openssl.h"
#include "crypto/sodium.h"

Encryptor::Encryptor(const std::string& password,
                     const Crypto::Cipher& cipher,
                     const bool& enable_ota)
    : chunk_id_(0), enable_ota_(enable_ota) {
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

bool Encryptor::Encrypt(std::vector<uint8_t>* ciphertext,
                        const std::vector<uint8_t>& plaintext) {
  std::vector<uint8_t> content;
  const std::vector<uint8_t>* payload =
      enable_ota_ ? &(content = plaintext) : &plaintext;

  if (enc_crypto_ == nullptr) {
    if (cipher_info_->library == Crypto::Library::OPENSSL) {
      enc_crypto_ = new CryptoOpenSSL(*cipher_info_, key_, enc_iv_,
                                      Crypto::OpCode::ENCRYPTION);
    } else if (cipher_info_->library == Crypto::Library::SODIUM) {
      enc_crypto_ = new CryptoSodium(*cipher_info_, key_, enc_iv_,
                                     Crypto::OpCode::ENCRYPTION);
    }

    if (enable_ota_) {
      content[0] |= 0x10;
      std::vector<uint8_t> key(enc_iv_), hmac(160);
      key.insert(key.end(), key_.begin(), key_.end());
      HMAC(EVP_sha1(), key.data(), key.size(), content.data(), content.size(),
           hmac.data(), nullptr);
      content.insert(content.end(), hmac.begin(), hmac.begin() + 10);
    }

    if (!enc_crypto_->Update(ciphertext, *payload)) {
      return false;
    }
    ciphertext->insert(ciphertext->begin(), enc_iv_.begin(), enc_iv_.end());

    return true;
  }

  if (enable_ota_) {
    std::vector<uint8_t> len(2), key(enc_iv_), hmac(160);
    *((uint16_t*)(len.data())) = htons(plaintext.size());
    key.resize(enc_iv_.size() + 4);
    *((uint32_t*)(key.data() + enc_iv_.size())) = htonl(chunk_id_++);
    HMAC(EVP_sha1(), key.data(), key.size(), content.data(), content.size(),
         hmac.data(), nullptr);
    content.insert(content.begin(), hmac.begin(), hmac.begin() + 10);
    content.insert(content.begin(), len.begin(), len.end());
  }

  return enc_crypto_->Update(ciphertext, *payload);
}

bool Encryptor::Decrypt(std::vector<uint8_t>* plaintext,
                        const std::vector<uint8_t>& ciphertext) {
  if (dec_crypto_ == nullptr) {
    if (ciphertext.size() < cipher_info_->iv_size) {
      return false;
    }

    dec_iv_.insert(dec_iv_.end(), ciphertext.begin(),
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

    return dec_crypto_->Update(plaintext, payload);
  }

  return dec_crypto_->Update(plaintext, ciphertext);
}

bool Encryptor::UpdateAll(const std::string& password,
                          const Crypto::Cipher& cipher,
                          std::vector<uint8_t>* out,
                          const std::vector<uint8_t>& in,
                          const Crypto::OpCode& enc,
                          const bool& enable_ota) {
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
  const std::vector<uint8_t>* payload = &content;
  if (enc == Crypto::OpCode::ENCRYPTION) {
    if (enable_ota) {
      content = in;
      content[0] |= 0x10;
      std::vector<uint8_t> hamc_key(iv), hmac(160);
      hamc_key.insert(hamc_key.end(), key.begin(), key.end());
      HMAC(EVP_sha1(), hamc_key.data(), hamc_key.size(), content.data(),
           content.size(), hmac.data(), nullptr);
      content.insert(content.end(), hmac.begin(), hmac.begin() + 10);
    } else {
      payload = &in;
      RAND_bytes(iv.data(), info->iv_size);
    }
  } else if (enc == Crypto::OpCode::DECRYPTION) {
    if (in.size() < info->iv_size) {
      return false;
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

  if (!crypto->Update(out, *payload)) {
    return false;
  }

  if (enc == Crypto::OpCode::ENCRYPTION) {
    out->insert(out->begin(), iv.begin(), iv.end());
  }

  delete crypto;
  return true;
}
