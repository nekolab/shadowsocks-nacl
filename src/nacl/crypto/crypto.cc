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


#include "crypto.h"


const std::map<std::string, Crypto::Cipher> Crypto::supported_cipher_({
  { "bf-cfb",    Crypto::Cipher::BF_CFB },
  { "rc2-cfb",   Crypto::Cipher::RC2_CFB },
  { "rc4-md5",   Crypto::Cipher::RC4_MD5 },
  { "idea-cfb",  Crypto::Cipher::IDEA_CFB },
  { "seed-cfb",  Crypto::Cipher::SEED_CFB },
  { "cast5-cfb", Crypto::Cipher::CAST5_CFB },
  { "salsa20",     Crypto::Cipher::SALSA20 },
  { "chacha20",    Crypto::Cipher::CHACHA20 },
  { "aes-128-cfb", Crypto::Cipher::AES_128_CFB },
  { "aes-192-cfb", Crypto::Cipher::AES_192_CFB },
  { "aes-256-cfb", Crypto::Cipher::AES_256_CFB },
  { "aes-128-ofb", Crypto::Cipher::AES_128_OFB },
  { "aes-192-ofb", Crypto::Cipher::AES_192_OFB },
  { "aes-256-ofb", Crypto::Cipher::AES_256_OFB },
  { "aes-128-ctr", Crypto::Cipher::AES_128_CTR },
  { "aes-192-ctr", Crypto::Cipher::AES_192_CTR },
  { "aes-256-ctr", Crypto::Cipher::AES_256_CTR },
  { "aes-128-cfb1", Crypto::Cipher::AES_128_CFB1 },
  { "aes-192-cfb1", Crypto::Cipher::AES_192_CFB1 },
  { "aes-256-cfb1", Crypto::Cipher::AES_256_CFB1 },
  { "aes-128-cfb8", Crypto::Cipher::AES_128_CFB8 },
  { "aes-192-cfb8", Crypto::Cipher::AES_192_CFB8 },
  { "aes-256-cfb8", Crypto::Cipher::AES_256_CFB8 },
  { "camellia-128-cfb", Crypto::Cipher::CAMELLIA_128_CFB },
  { "camellia-192-cfb", Crypto::Cipher::CAMELLIA_192_CFB },
  { "camellia-256-cfb", Crypto::Cipher::CAMELLIA_256_CFB }
});


const std::map<Crypto::Cipher, Crypto::CipherInfo> Crypto::cipher_details_({
  { Crypto::Cipher::RC4_MD5,          { 16, 16, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_rc4 } },
  { Crypto::Cipher::BF_CFB,           {  8, 16, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_bf_cfb } },
  { Crypto::Cipher::RC2_CFB,          {  8, 16, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_rc2_cfb } },
  { Crypto::Cipher::IDEA_CFB,         {  8, 16, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_idea_cfb } },
  { Crypto::Cipher::SEED_CFB,         { 16, 16, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_seed_cfb } },
  { Crypto::Cipher::CAST5_CFB,        {  8, 16, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_cast5_cfb } },
  { Crypto::Cipher::AES_128_CFB,      { 16, 16, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_aes_128_cfb } },
  { Crypto::Cipher::AES_192_CFB,      { 16, 24, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_aes_192_cfb } },
  { Crypto::Cipher::AES_256_CFB,      { 16, 32, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_aes_256_cfb } },
  { Crypto::Cipher::AES_128_OFB,      { 16, 16, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_aes_128_ofb } },
  { Crypto::Cipher::AES_192_OFB,      { 16, 24, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_aes_192_ofb } },
  { Crypto::Cipher::AES_256_OFB,      { 16, 32, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_aes_256_ofb } },
  { Crypto::Cipher::AES_128_CTR,      { 16, 16, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_aes_128_ctr } },
  { Crypto::Cipher::AES_192_CTR,      { 16, 24, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_aes_192_ctr } },
  { Crypto::Cipher::AES_256_CTR,      { 16, 32, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_aes_256_ctr } },
  { Crypto::Cipher::AES_128_CFB1,     { 16, 16, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_aes_128_cfb1 } },
  { Crypto::Cipher::AES_192_CFB1,     { 16, 24, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_aes_192_cfb1 } },
  { Crypto::Cipher::AES_256_CFB1,     { 16, 32, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_aes_256_cfb1 } },
  { Crypto::Cipher::AES_128_CFB8,     { 16, 16, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_aes_128_cfb8 } },
  { Crypto::Cipher::AES_192_CFB8,     { 16, 24, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_aes_192_cfb8 } },
  { Crypto::Cipher::AES_256_CFB8,     { 16, 32, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_aes_256_cfb8 } },
  { Crypto::Cipher::CAMELLIA_128_CFB, { 16, 16, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_camellia_128_cfb } },
  { Crypto::Cipher::CAMELLIA_192_CFB, { 16, 24, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_camellia_192_cfb } },
  { Crypto::Cipher::CAMELLIA_256_CFB, { 16, 32, Crypto::Library::OPENSSL, .openssl_cipher = &EVP_camellia_256_cfb } },
  { Crypto::Cipher::SALSA20,          {  8, 32, Crypto::Library::SODIUM,  .sodium_cipher  = &crypto_stream_salsa20_xor_ic } },
  { Crypto::Cipher::CHACHA20,         {  8, 32, Crypto::Library::SODIUM,  .sodium_cipher  = &crypto_stream_chacha20_xor_ic } }
});


const Crypto::Cipher *Crypto::GetCipher(std::string name) {
  auto iter = Crypto::supported_cipher_.find(name);
  return (iter == Crypto::supported_cipher_.end()) ? nullptr : &(iter->second);
}


const Crypto::CipherInfo *Crypto::GetCipherInfo(Crypto::Cipher cipher) {
  auto iter = Crypto::cipher_details_.find(cipher);
  return (iter == Crypto::cipher_details_.end()) ? nullptr : &(iter->second);
}


std::vector<std::string> Crypto::GetSupportedCipherNames() {
  std::vector<std::string> v;
  for (auto info : Crypto::supported_cipher_) {
    v.push_back(info.first);
  }
  return v;
}
